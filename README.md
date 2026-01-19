# Inference-Core

Motor de inferencia de alto rendimiento optimizado para GPUs de recursos limitados (GTX 1060 3GB), utilizando `llama.cpp` integrado directamente en C++ para máxima eficiencia y mínima latencia.

## Estado Actual (Fase 1: Core Engine)

El proyecto se encuentra en una etapa temprana pero funcional. El núcleo de inferencia (`Engine`) ha sido implementado y probado con éxito.

### Características Implementadas
- **Core C++ Nativo**: Wrapper robusto sobre la API de C de `llama.cpp` sin dependencias de alto nivel (Python/Node).
- **Aceleración GPU Manual**: Configuración reforzada de CMake para garantizar el uso de CUDA (cuBLAS).
- **Gestión de Memoria**: Sistema de carga inteligente que aprovecha la VRAM disponible.
- **CLI de Prueba**: Ejecutable `InferenceCore` capaz de cargar modelos GGUF y generar texto por consola.

### Estructura del Proyecto
```
.
├── src/
│   ├── core/           # Motor de inferencia (Engine.cpp/h)
│   ├── server/         # (Futuro) Servidor WebSocket
│   ├── hardware/       # (Futuro) Monitorización NVML
│   └── main.cpp        # CLI actual p/ pruebas
├── llama.cpp/          # Submódulo: Núcleo de computación tensorial
├── CMakeLists.txt      # Configuración de compilación con soporte CUDA forzado
└── README.md           # Este archivo
```

## Requisitos
- **Drivers NVIDIA** (Testado con versión 535+)
- **CUDA Toolkit** (Testado con versión 12.0)
- **Compilador C++** (GCC/Clang compatibles con C++17)
- **CMake** 3.14+

## Compilación y Uso

```bash
mkdir build && cd build
cmake ..
make -j4
./InferenceCore /ruta/a/tu/modelo.gguf
```

## Roadmap Inmediato
- [x] Fase 1: Core Engine
- [ ] Fase 2: Servidor WebSocket (Zero-Latency Streaming)
- [ ] Fase 3: Monitorización Hardware (NVML)
- [ ] Fase 4: Producción (Systemd, Watchdogs)
