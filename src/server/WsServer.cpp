#include "WsServer.h"
#include <chrono>

namespace {
    // Helper: Validate and clean UTF-8 string to prevent JSON serialization errors
    std::string sanitizeUtf8(const std::string& input) {
        std::string output;
        output.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); ) {
            unsigned char c = input[i];
            if (c < 0x80) {
                // ASCII
                output += c;
                i++;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < input.size()) {
                // 2-byte UTF-8
                output += input.substr(i, 2);
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < input.size()) {
                // 3-byte UTF-8
                output += input.substr(i, 3);
                i += 3;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < input.size()) {
                // 4-byte UTF-8
                output += input.substr(i, 4);
                i += 4;
            } else {
                // Invalid UTF-8, skip this byte
                i++;
            }
        }
        return output;
    }
}

namespace Server {

    WsServer::WsServer(Core::Engine& engine, Hardware::Monitor& monitor, int port) 
        : engine(engine), monitor(monitor), port(port) {
        
        // Start worker thread
        workerThread = std::thread(&WsServer::workerLoop, this);
        
        // Start metrics broadcasting thread
        metricsThread = std::thread(&WsServer::metricsLoop, this);
    }

    WsServer::~WsServer() {
        running = false;
        queueCv.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
        if (metricsThread.joinable()) {
            metricsThread.join();
        }
    }

    void WsServer::run() {
        struct uWS::Loop* loop = uWS::Loop::get(); 
        this->loop = loop;

        uWS::App()
            .ws<PerSocketData>("/*", {
                .open = [this](auto* ws) {
                    std::cout << "Client connected!" << std::endl;
                    
                    // Add to connected clients
                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        connectedClients.insert(ws);
                    }
                    
                    json hello = {
                        {"op", Op::HELLO},
                        {"status", "ready"}
                    };
                    ws->send(hello.dump(), uWS::OpCode::TEXT);
                },
                .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
                    try {
                        auto j = json::parse(message);
                        if (!j.contains("op")) return;

                        std::string op = j["op"];
                        
                        if (op == Op::INFER) {
                            InferenceParams params = parseInfer(j);
                            
                            std::lock_guard<std::mutex> lock(queueMutex);
                            taskQueue.push({ws, params});
                            queueCv.notify_one();
                        }
                        else if (op == Op::ABORT) {
                            if (currentWs == ws) {
                                abortCurrent = true;
                            }
                        }

                    } catch (const std::exception& e) {
                        std::cerr << "JSON Error: " << e.what() << std::endl;
                    }
                },
                .close = [this](auto* ws, int code, std::string_view message) {
                    std::cout << "Client disconnected" << std::endl;
                    
                    // Remove from connected clients
                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        connectedClients.erase(ws);
                    }
                    
                    if (currentWs == ws) {
                        abortCurrent = true;
                        currentWs = nullptr;
                    }
                }
            })
            .listen(port, [this](auto* token) {
                if (token) {
                    std::cout << "Listening on port " << port << std::endl;
                } else {
                    std::cerr << "Failed to listen on port " << port << std::endl;
                }
            })
            .run();
    }

    void WsServer::workerLoop() {
        while (running) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCv.wait(lock, [this] { return !taskQueue.empty() || !running; });

                if (!running) break;

                task = taskQueue.front();
                taskQueue.pop();
            }

            processInference(task);
        }
    }

    void WsServer::processInference(Task& task) {
        currentWs = task.ws;
        abortCurrent = false;
        isGenerating = true;

        auto metrics = engine.generate(task.params.prompt, [this, &task](const std::string& token) {
            // Check abort
            if (abortCurrent) return false;

            // Send to main thread
            std::string tokenCopy = token;
            
            loop->defer([this, task, tokenCopy]() {
                // Verify if connection is still alive/active
                if (currentWs == task.ws && currentWs != nullptr) {
                     // Sanitize UTF-8 to prevent JSON serialization errors
                     std::string validToken = sanitizeUtf8(tokenCopy);
                     
                     json msg = {
                        {"op", Op::TOKEN},
                        {"content", validToken}
                     };
                     currentWs->send(msg.dump(), uWS::OpCode::TEXT);
                }
            });
            
            return true;
        });

        // Store metrics for broadcasting
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            lastMetrics = metrics;
        }

        // Finished
        loop->defer([this, task, metrics]() {
            if (currentWs == task.ws && currentWs != nullptr) {
                 json msg = {
                    {"op", Op::END},
                    {"stats", {
                        {"ttft_ms", metrics.ttft_ms},
                        {"total_ms", metrics.total_time_ms},
                        {"tokens", metrics.tokens_generated},
                        {"tps", metrics.tps}
                    }}
                 };
                 currentWs->send(msg.dump(), uWS::OpCode::TEXT);
                 
                 // Reset current
                 currentWs = nullptr;
            }
        });
        
        isGenerating = false;
    }
    
    void WsServer::metricsLoop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            if (!running) break;
            
            broadcastMetrics();
        }
    }
    
    void WsServer::broadcastMetrics() {
        // Get current GPU stats
        auto gpuStats = monitor.updateStats();
        
        // Get current metrics
        Core::Metrics currentMetrics;
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            currentMetrics = lastMetrics;
        }
        
        // Build metrics JSON
        json metricsJson = {
            {"op", Op::METRICS},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"gpu", {
                {"temp", gpuStats.temp},
                {"vram_total_mb", gpuStats.memoryTotal / (1024*1024)},
                {"vram_used_mb", gpuStats.memoryUsed / (1024*1024)},
                {"vram_free_mb", gpuStats.memoryFree / (1024*1024)},
                {"power_watts", gpuStats.powerUsage / 1000}, // Convert mW to W
                {"fan_percent", gpuStats.fanSpeed},
                {"throttling", gpuStats.throttle}
            }},
            {"inference", {
                {"state", isGenerating.load() ? "generating" : "idle"},
                {"last_tps", currentMetrics.tps},
                {"last_ttft_ms", currentMetrics.ttft_ms},
                {"total_tokens_generated", currentMetrics.tokens_generated}
            }}
        };
        
        std::string metricsStr = metricsJson.dump();
        
        // Broadcast to all connected clients
        loop->defer([this, metricsStr]() {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto* ws : connectedClients) {
                ws->send(metricsStr, uWS::OpCode::TEXT);
            }
        });
    }
}
