#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Server {

    // Opcodes for client -> server messages
    namespace Op {
        constexpr const char* INFER = "infer";
        constexpr const char* ABORT = "abort";
        
        // Server -> Client
        constexpr const char* HELLO = "hello";
        constexpr const char* TOKEN = "token";
        constexpr const char* END   = "end";
        constexpr const char* ERROR = "error";
    }

    struct InferenceParams {
        std::string prompt;
        float temp = 0.7f;
        int max_tokens = -1;
    };

    inline InferenceParams parseInfer(const json& payload) {
        InferenceParams p;
        if (payload.contains("prompt")) p.prompt = payload["prompt"].get<std::string>();
        if (payload.contains("params")) {
            auto& params = payload["params"];
            if (params.contains("temp")) p.temp = params["temp"].get<float>();
            if (params.contains("max_tokens")) p.max_tokens = params["max_tokens"].get<int>();
        }
        return p;
    }

}
