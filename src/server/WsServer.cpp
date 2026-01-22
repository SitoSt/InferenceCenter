#include "WsServer.h"

namespace Server {

    WsServer::WsServer(Core::Engine& engine, int port) 
        : engine(engine), port(port) {
        
        // Start worker thread
        workerThread = std::thread(&WsServer::workerLoop, this);
    }

    WsServer::~WsServer() {
        running = false;
        queueCv.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    void WsServer::run() {
        struct uWS::Loop* loop = uWS::Loop::get(); 
        this->loop = loop;

        uWS::App()
            .ws<PerSocketData>("/*", {
                .open = [](auto* ws) {
                    std::cout << "Client connected!" << std::endl;
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

        auto metrics = engine.generate(task.params.prompt, [this, &task](const std::string& token) {
            // Check abort
            if (abortCurrent) return false;

            // Send to main thread
            // Copy token because 'token' ref might be invalid when defer runs
            std::string tokenCopy = token;
            
            loop->defer([this, task, tokenCopy]() {
                // Verify if connection is still alive/active
                if (currentWs == task.ws && currentWs != nullptr) {
                     json msg = {
                        {"op", Op::TOKEN},
                        {"content", tokenCopy}
                     };
                     currentWs->send(msg.dump(), uWS::OpCode::TEXT);
                }
            });
            
            return true;
        });

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
    }
}
