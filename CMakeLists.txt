cmake_minimum_required(VERSION 3.18)
project(InferenceCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Buscar CUDA
find_package(CUDAToolkit REQUIRED)

# 2. IMPORTAR llama.cpp (Esto es la clave)
# En lugar de buscar archivos .a, añadimos el subdirectorio.
# Esto hace que las "metas" (targets) de llama estén disponibles.
set(GGML_CUDA ON CACHE BOOL "Enable CUDA" FORCE)
add_subdirectory(llama.cpp)

find_package(Threads REQUIRED)

# 3. Tu ejecutable
add_executable(${PROJECT_NAME} src/main.cpp)

# 4. Enlazar usando los nombres de los objetivos, no rutas a archivos
# Esto gestiona automáticamente las cabeceras (.h) y los archivos (.a)
target_link_libraries(${PROJECT_NAME} PRIVATE 
llama 
    ggml 
    CUDA::cudart
    CUDA::cublas
    Threads::Threads    # <--- Esta es la forma correcta en CMake
    ${CMAKE_DL_LIBS}    # <--- Forma más segura de poner 'dl'
)

# 5. Flags
if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O3 -march=native)
endif()