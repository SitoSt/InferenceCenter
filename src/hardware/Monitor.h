#pragma once

#include <string>

#ifdef USE_CUDA
#include <nvml.h>
#endif

namespace Hardware {

    struct GpuStats {
        unsigned int temp;           // Temperature in C
        unsigned long long memoryTotal; // Bytes
        unsigned long long memoryFree;  // Bytes
        unsigned long long memoryUsed;  // Bytes
        unsigned int powerUsage;     // Milliwatts
        unsigned int fanSpeed;       // %
        bool throttle;               // True if temp > 80C
    };

    class Monitor {
    public:
        Monitor();
        ~Monitor();

        bool init();
        void shutdown();
        
        // Updates internal stats and returns them
        GpuStats updateStats();
        
        // Quick check without full update
        bool isThrottling() const;
        
        // Calculate optimal number of GPU layers based on available VRAM
        // Returns -1 if CUDA not available, otherwise recommended layer count
        int calculateOptimalGpuLayers(unsigned long long modelSizeBytes);

    private:
#ifdef USE_CUDA
        nvmlDevice_t device;
#endif
        bool initialized = false;
        GpuStats currentStats;
        
        // Constants (GTX 1060 constraints)
        const unsigned int MAX_TEMP_SAFE = 80;
    };

}
