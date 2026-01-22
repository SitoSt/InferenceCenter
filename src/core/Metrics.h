#pragma once
#include <string>
#include <chrono>

namespace Core {
    struct Metrics {
        // Time To First Token (ms)
        long long ttft_ms = 0;
        // Total generation time (ms) (excluding prompt processing if possible, or total)
        long long total_time_ms = 0;
        // Tokens generated
        int tokens_generated = 0;
        // Tokens Per Second
        double tps = 0.0;
        
        // Resource Usage (Placeholder for now)
        // Memory used, etc.
    };
}
