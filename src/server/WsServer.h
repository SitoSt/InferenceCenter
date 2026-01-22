#pragma once

#include <App.h> // uWebSockets
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include "Protocol.h"
#include "../core/Engine.h"

namespace Server {

    struct PerSocketData {
        // Can hold user session state
    };

    class WsServer {
    public:
        WsServer(Core::Engine& engine, int port = 3000);
        ~WsServer();

        // Start the server loop (blocking)
        void run();

    private:
        Core::Engine& engine;
        int port;
        
        // uWebSockets LOOP
        // We need to access the loop to defer tasks from the worker thread
        struct uWS::Loop* loop = nullptr;
        
        // Thread Safe Queue for Inference Tasks
        struct Task {
            uWS::WebSocket<false, true, PerSocketData>* ws;
            InferenceParams params;
        };
        
        std::queue<Task> taskQueue;
        std::mutex queueMutex;
        std::condition_variable queueCv;
        
        std::atomic<bool> running{true};
        std::thread workerThread;

        // Current active generation (to support ABORT)
        std::atomic<bool> abortCurrent{false};
        uWS::WebSocket<false, true, PerSocketData>* currentWs = nullptr;

        void workerLoop();
        void processInference(Task& task);
    };

}
