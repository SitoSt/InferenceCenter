#include "Monitor.h"
#include <iostream>

namespace Hardware {

    Monitor::Monitor() {
        // Initialize stats to 0
        currentStats = {};
    }

    Monitor::~Monitor() {
        shutdown();
    }

    bool Monitor::init() {
#ifdef USE_CUDA
        nvmlReturn_t result = nvmlInit();
        if (NVML_SUCCESS != result) {
            std::cerr << "Failed to initialize NVML: " << nvmlErrorString(result) << std::endl;
            return false;
        }

        // Get handle for first device (index 0)
        // Assuming single GPU setup (GTX 1060)
        result = nvmlDeviceGetHandleByIndex(0, &device);
        if (NVML_SUCCESS != result) {
            std::cerr << "Failed to get device handle: " << nvmlErrorString(result) << std::endl;
            nvmlShutdown();
            return false;
        }

        // Verify device name
        char name[64];
        if (NVML_SUCCESS == nvmlDeviceGetName(device, name, sizeof(name))) {
            std::cout << "Monitor initialized for: " << name << std::endl;
        }

        initialized = true;
        return true;
#else
        std::cout << "Monitor: CUDA support not compiled. Running in CPU-only mode." << std::endl;
        return false;
#endif
    }

    void Monitor::shutdown() {
#ifdef USE_CUDA
        if (initialized) {
            nvmlShutdown();
            initialized = false;
        }
#endif
    }

    GpuStats Monitor::updateStats() {
        if (!initialized) return currentStats;

#ifdef USE_CUDA
        nvmlReturn_t result;
        nvmlMemory_t memory;
        unsigned int temp;
        unsigned int power;
        unsigned int fan;

        // Memory
        result = nvmlDeviceGetMemoryInfo(device, &memory);
        if (NVML_SUCCESS == result) {
            currentStats.memoryTotal = memory.total;
            currentStats.memoryFree = memory.free;
            currentStats.memoryUsed = memory.used;
        }

        // Temperature
        result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
        if (NVML_SUCCESS == result) {
            currentStats.temp = temp;
        }

        // Power
        result = nvmlDeviceGetPowerUsage(device, &power);
        if (NVML_SUCCESS == result) {
            currentStats.powerUsage = power;
        }

        // Fan Speed
        result = nvmlDeviceGetFanSpeed(device, &fan);
        if (NVML_SUCCESS == result) {
            currentStats.fanSpeed = fan;
        }

        // Throttle Check
        if (currentStats.temp >= MAX_TEMP_SAFE) {
            if (!currentStats.throttle) {
                std::cerr << "WARNING: GPU Temperature " << currentStats.temp << "C exceeds limit " << MAX_TEMP_SAFE << "C. Throttling..." << std::endl;
            }
            currentStats.throttle = true;
        } else {
            if (currentStats.throttle) {
                std::cout << "INFO: GPU Temperature normalized (" << currentStats.temp << "C)." << std::endl;
            }
            currentStats.throttle = false;
        }
#endif

        return currentStats;
    }

    bool Monitor::isThrottling() const {
        return currentStats.throttle;
    }

    int Monitor::calculateOptimalGpuLayers(unsigned long long modelSizeBytes) {
#ifdef USE_CUDA
        if (!initialized) {
            std::cerr << "Monitor not initialized. Cannot calculate GPU layers." << std::endl;
            return -1;
        }

        // Update stats to get current VRAM
        updateStats();

        // Safety buffer: 500MB to prevent OOM
        const unsigned long long SAFETY_BUFFER_MB = 500;
        const unsigned long long SAFETY_BUFFER_BYTES = SAFETY_BUFFER_MB * 1024 * 1024;

        // Calculate available VRAM with buffer
        unsigned long long availableVRAM = 0;
        if (currentStats.memoryFree > SAFETY_BUFFER_BYTES) {
            availableVRAM = currentStats.memoryFree - SAFETY_BUFFER_BYTES;
        } else {
            std::cerr << "WARNING: Insufficient VRAM available. Free: " 
                      << (currentStats.memoryFree / (1024*1024)) << " MB" << std::endl;
            return 0; // No GPU layers
        }

        std::cout << "\n--- Smart Split Computing ---" << std::endl;
        std::cout << "VRAM Total: " << (currentStats.memoryTotal / (1024*1024)) << " MB" << std::endl;
        std::cout << "VRAM Free:  " << (currentStats.memoryFree / (1024*1024)) << " MB" << std::endl;
        std::cout << "Safety Buffer: " << SAFETY_BUFFER_MB << " MB" << std::endl;
        std::cout << "Available for Model: " << (availableVRAM / (1024*1024)) << " MB" << std::endl;

        // If entire model fits in VRAM, use all layers (return 99 = max)
        if (modelSizeBytes <= availableVRAM) {
            std::cout << "Model fits entirely in GPU (" << (modelSizeBytes / (1024*1024)) 
                      << " MB). Using all layers." << std::endl;
            std::cout << "-----------------------------\n" << std::endl;
            return 99; // Max layers
        }

        // Estimate layers based on proportion
        // Typical 7B model has ~32 layers, 3B has ~28 layers, 1B has ~22 layers
        // We'll estimate based on model size
        int estimatedTotalLayers = 32; // Default assumption for 7B models
        if (modelSizeBytes < 2ULL * 1024 * 1024 * 1024) { // < 2GB
            estimatedTotalLayers = 22; // Small model (1B)
        } else if (modelSizeBytes < 4ULL * 1024 * 1024 * 1024) { // < 4GB
            estimatedTotalLayers = 28; // Medium model (3B)
        }

        // Calculate proportion of layers that fit
        double proportion = (double)availableVRAM / (double)modelSizeBytes;
        int recommendedLayers = (int)(proportion * estimatedTotalLayers);

        // Ensure at least 1 layer on GPU if there's any VRAM available
        if (recommendedLayers < 1 && availableVRAM > 0) {
            recommendedLayers = 1;
        }

        std::cout << "Model size: " << (modelSizeBytes / (1024*1024)) << " MB" << std::endl;
        std::cout << "Estimated total layers: " << estimatedTotalLayers << std::endl;
        std::cout << "Recommended GPU layers: " << recommendedLayers 
                  << " (" << (int)(proportion * 100) << "% of model)" << std::endl;
        std::cout << "Remaining layers will use CPU" << std::endl;
        std::cout << "-----------------------------\n" << std::endl;

        return recommendedLayers;
#else
        std::cout << "CUDA not compiled. Cannot use GPU layers." << std::endl;
        return 0; // CPU-only
#endif
    }

}
