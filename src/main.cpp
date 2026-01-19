#include <iostream>
#include "Engine.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_model.gguf>" << std::endl;
        return 1;
    }

    std::string modelPath = argv[1];

    Core::Engine engine;
    
    std::cout << "--- INFERENCE CORE ---" << std::endl;
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

    std::string prompt = "Hello, I am an AI assistant. I can help you with";
    std::cout << "\nPrompt: " << prompt << "\nGenerating...\n" << std::endl;

    engine.generate(prompt, [](const std::string& token) {
        std::cout << token << std::flush;
        return true; // Continue
    });

    std::cout << "\n\nGeneration complete." << std::endl;
    return 0;
}