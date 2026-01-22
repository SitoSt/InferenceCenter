#include <iostream>
#include "Engine.h"
#include "WsServer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_model.gguf> [port]" << std::endl;
        return 1;
    }

    std::string modelPath = argv[1];
    int port = 3000;
    if (argc >= 3) {
        port = std::atoi(argv[2]);
    }

    // 1. Initialize Engine
    Core::Engine engine;
    
    std::cout << "--- INFERENCE CORE SERVER ---" << std::endl;
    std::cout << engine.getSystemInfo() << std::endl;

    Core::EngineConfig config;
    config.modelPath = modelPath;
    config.n_gpu_layers = 99; // Try to offload everything

    std::cout << "Loading model: " << modelPath << "..." << std::endl;
    if (!engine.loadModel(config)) {
        std::cerr << "Failed to load model." << std::endl;
        return 1;
    }
    std::cout << "Model loaded successfully." << std::endl;

    // 2. Start WebSocket Server
    Server::WsServer server(engine, port);
    server.run();

    return 0;
}