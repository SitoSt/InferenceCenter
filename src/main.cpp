#include <iostream>
#include "llama.h"

int main() {
    // 1. Inicializar
    llama_backend_init();

    // 2. Comprobar capacidades de hardware
    bool has_cuda = llama_supports_gpu_offload();
    
    std::cout << "--- DIAGNÓSTICO DE INFERENCECORE ---" << std::endl;
    std::cout << "Soporte CUDA compilado: " << (has_cuda ? "SÍ" : "NO") << std::endl;
    // printf("Llama.cpp version: %d\n", llama_libraries_version());
    std::cout << "System Info: " << llama_print_system_info() << std::endl;

    if (!has_cuda) {
        std::cout << "ERROR: El motor no ve la GPU. Revisa si nvidia-smi funciona." << std::endl;
    } else {
        std::cout << "¡ÉXITO! La GTX 1060 está lista para la acción." << std::endl;
    }

    llama_backend_free();
    return 0;
}