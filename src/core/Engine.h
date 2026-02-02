#pragma once

#include "llama.h"
#include "Metrics.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

namespace Core {

    // Callback for streaming tokens. Returns true to continue, false to abort.
    using TokenCallback = std::function<bool(const std::string& token)>;

    struct EngineConfig {
        std::string modelPath;
        int n_gpu_layers = -1; // -1 = auto-detect, 0 = CPU only, >0 = specific count
        int ctx_size = 512;    // Reduced for short conversations
        bool use_mmap = true;
        bool use_mlock = false;
    };

    class Engine {
    public:
        Engine();
        ~Engine();

        // Prevent copying
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Initialize the engine with the specific model
        bool loadModel(const EngineConfig& config);

        // Run inference
        // prompt: The input text
        // callback: Function called for each generated token
        // Returns final metrics
        Metrics generate(const std::string& prompt, TokenCallback callback);

        // Check internal status
        bool isLoaded() const;

        // Get system info
        std::string getSystemInfo() const;

    private:
        struct llama_model* model = nullptr;
        struct llama_context* ctx = nullptr;
        
        // Helper to tokenize input
        std::vector<llama_token> tokenize(const std::string & text, bool add_bos);
        
        // Helper to convert token to string
        std::string tokenToPiece(llama_token token);
    };

}
