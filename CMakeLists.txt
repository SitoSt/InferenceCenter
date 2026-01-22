cmake_minimum_required(VERSION 3.18)
project(InferenceCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Buscar CUDA
find_package(CUDAToolkit REQUIRED)
find_package(ZLIB REQUIRED) # Needed by uWebSockets

# 2. IMPORTAR llama.cpp (Esto es la clave)
# En lugar de buscar archivos .a, añadimos el subdirectorio.
# Esto hace que las "metas" (targets) de llama estén disponibles.
set(GGML_CUDA ON CACHE BOOL "Enable CUDA" FORCE)
add_subdirectory(llama.cpp)

find_package(Threads REQUIRED)

# --- DEPENDENCIES (FetchContent) ---
include(FetchContent)

# 1. JSON (nlohmann/json)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# 2. uSockets (Base layer for uWebSockets)
FetchContent_Declare(
    usockets
    GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
    GIT_TAG        v0.8.8
)
# uSockets is a bit tricky with CMake sometimes, but let's try standard approach or just source inclusion if needed.
# Usually uWebSockets handles uSockets inclusion internally if not provided, 
# but providing it explicitly gives more control. For simplicity in this env,
# we might rely on uWebSockets finding it or including it if we encounter issues.
# For now, let's use a known working fork or just uWebSockets strict.
# Actually, uWebSockets v20+ is header only but needs uSockets.o linked.

FetchContent_Declare(
    uwebsockets
    GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
    GIT_TAG        v20.64.0
)
# FetchContent_MakeAvailable(usockets uwebsockets) <-- uSockets has no CMakeLists.txt!

# 2.1 Manually configure uSockets
FetchContent_GetProperties(usockets)
if(NOT usockets_POPULATED)
  FetchContent_Populate(usockets)
endif()

# Create uSockets library from source
file(GLOB_RECURSE USOCKETS_SOURCES "${usockets_SOURCE_DIR}/src/*.c" "${usockets_SOURCE_DIR}/src/*.cpp")

# Exclude main files or examples if any (usually src is clean)
add_library(uSockets STATIC ${USOCKETS_SOURCES})

# Disable SSL for simplicity (avoids OpenSSL dependency hell for now)
target_compile_definitions(uSockets PUBLIC LIBUS_NO_SSL)
target_include_directories(uSockets PUBLIC "${usockets_SOURCE_DIR}/src")

# 2.2 Configure uWebSockets
FetchContent_GetProperties(uwebsockets)
if(NOT uwebsockets_POPULATED)
  FetchContent_Populate(uwebsockets)
endif()
# uWebSockets is header only, just needs the includes (handled below)

# --- END DEPENDENCIES ---

# 3. Tu ejecutable
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/core/Engine.cpp
    src/server/Protocol.h
    src/server/WsServer.h
    src/server/WsServer.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE src/core src/server)

# 4. Enlazar usando los nombres de los objetivos, no rutas a archivos
# Esto gestiona automáticamente las cabeceras (.h) y los archivos (.a)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    llama 
    ggml 
    uSockets
    nlohmann_json::nlohmann_json
    ZLIB::ZLIB
    CUDA::cudart
    CUDA::cublas
    Threads::Threads    # <--- Esta es la forma correcta en CMake
    ${CMAKE_DL_LIBS}    # <--- Forma más segura de poner 'dl'
)

# uWebSockets is header only, so just include directory
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${uwebsockets_SOURCE_DIR}/src
    ${usockets_SOURCE_DIR}/src
)

# 5. Flags
if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O3 -march=native)
endif()