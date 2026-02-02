# InferenceCenter

Motor de inferencia de alto rendimiento optimizado para GPUs de recursos limitados (GTX 1060 3GB), utilizando `llama.cpp` integrado directamente en C++ para máxima eficiencia y mínima latencia.

## 🚀 Quick Start

### Compilación Rápida (CPU-Only)

```bash
# Instalar dependencias
sudo apt-get install -y build-essential cmake git zlib1g-dev

# Compilar
cd /home/sito/InferenceCenter
mkdir build && cd build
cmake -DUSE_CUDA=OFF ..
make -j$(nproc)

# Ejecutar (necesitas un modelo .gguf)
./InferenceCore /ruta/a/modelo.gguf 3000
```

### Compilación con GPU (Recomendado)

```bash
# 1. Instalar CUDA y drivers
sudo apt-get install -y nvidia-driver-535 nvidia-cuda-toolkit
sudo reboot

# 2. Compilar con soporte GPU
cd /home/sito/InferenceCenter
mkdir build && cd build
cmake -DUSE_CUDA=ON ..
make -j$(nproc)

# 3. Ejecutar
./InferenceCore /ruta/a/modelo.gguf 3000
```

> **📖 Documentación completa**: Ver [BUILD.md](BUILD.md) para instrucciones detalladas, requisitos y solución de problemas.

---

## 📊 Estado Actual

### ✅ Fase 1: Hardware Monitoring (Completada)

- **Core C++ Nativo**: Wrapper robusto sobre la API de C de `llama.cpp`
- **Monitorización GPU**: Clase `Monitor` con NVML para rastrear VRAM, temperatura y throttling
- **Compilación Dual**: Soporte para CPU-only y GPU-enabled
- **WebSocket Server**: Servidor en tiempo real con streaming de tokens
- **Protocolo JSON**: Operaciones `INFER`, `ABORT`, `TOKEN`, `END`

### ✅ Fase 2: Smart Split Computing + Metrics (Completada)

- **Auto-detección GPU**: Calcula automáticamente cuántas capas cargar en GPU según VRAM disponible
- **Buffer de seguridad**: Reserva 500MB para prevenir OOM
- **Contexto optimizado**: Reducido a 512 tokens para conversaciones cortas
- **Métricas en tiempo real**: Broadcasting cada 1 segundo con stats de GPU e inferencia
- **Operación METRICS**: Nueva operación WebSocket para dashboards externos

> **📖 Ver**: [docs/PHASE2_WALKTHROUGH.md](docs/PHASE2_WALKTHROUGH.md) para detalles de implementación

### 🔄 Próximas Fases

- [ ] **Fase 3**: Watchdog (detección de cuelgues y auto-recuperación)
- [ ] **Fase 4**: Cliente Web (UI para testing y visualización de métricas)
- [ ] **Fase 5**: Producción (systemd, logging estructurado)

---

## 🏗️ Arquitectura

```
src/
├── core/
│   ├── Engine.cpp/.h        # Motor de inferencia (llama.cpp wrapper)
│   └── Allocator.h          # (Futuro) Gestión de KV Cache
├── server/
│   ├── WsServer.cpp/.h      # Servidor WebSocket (uWebSockets)
│   ├── Protocol.h           # Definiciones de protocolo JSON
│   └── Client.h             # Estado por cliente
├── hardware/
│   └── Monitor.cpp/.h       # Monitorización NVML (GPU stats)
└── main.cpp                 # Punto de entrada
```

---

## 🔧 Opciones de Compilación

| Modo | Flag CMake | Requisitos | Velocidad | Monitorización |
|------|------------|------------|-----------|----------------|
| **CPU-Only** | `-DUSE_CUDA=OFF` | GCC, CMake | Lenta | ❌ |
| **GPU** | `-DUSE_CUDA=ON` | CUDA, Drivers NVIDIA | 10-50x más rápida | ✅ |

---

## 📡 Protocolo WebSocket

### Cliente → Servidor

```json
{
  "op": "infer",
  "prompt": "Explica la teoría de la relatividad",
  "params": {
    "temp": 0.7,
    "max_tokens": 500
  }
}
```

### Servidor → Cliente (Streaming)

```json
{"op": "token", "content": " La"}
{"op": "token", "content": " teoría"}
{"op": "token", "content": " de"}
...
{
  "op": "end",
  "stats": {
    "ttft_ms": 45,
    "total_ms": 1200,
    "tokens": 87,
    "tps": 72.5
  }
}
```

### Servidor → Cliente (Métricas en Tiempo Real)

Cada 1 segundo, el servidor envía automáticamente:

```json
{
  "op": "metrics",
  "timestamp": 1706735234,
  "gpu": {
    "temp": 65,
    "vram_total_mb": 3072,
    "vram_used_mb": 1850,
    "vram_free_mb": 1222,
    "power_watts": 85,
    "fan_percent": 60,
    "throttling": false
  },
  "inference": {
    "state": "idle",
    "last_tps": 45.2,
    "last_ttft_ms": 120,
    "total_tokens_generated": 1523
  }
}
```

---

## 🎯 Modelos Recomendados (GTX 1060 3GB)

| Modelo | Cuantización | Tamaño | Rendimiento |
|--------|--------------|--------|-------------|
| Llama-3.2-1B | q4_k_m | ~800MB | ✅ Excelente |
| Llama-3.2-3B | q4_k_m | ~2.1GB | ✅ Muy bueno |
| Mistral-7B | q4_k_m | ~4.8GB | ⚠️ Requiere split CPU/GPU |
| Llama-3-8B | q4_k_m | ~5.2GB | ⚠️ Requiere split CPU/GPU |

Descarga modelos desde: https://huggingface.co/models?library=gguf

---

## 🧪 Testing

### Test con Cliente Python

```bash
# Instalar dependencias
pip install websockets

# Ejecutar cliente de prueba
python3 test_client.py
```

### Test Manual (wscat)

```bash
# Instalar wscat
npm install -g wscat

# Conectar
wscat -c ws://localhost:3000

# Enviar prompt
{"op":"infer","prompt":"Hola, ¿cómo estás?"}
```

---

## 📚 Documentación

### Guías de Usuario
- **[BUILD.md](BUILD.md)**: Guía completa de compilación e instalación
- **[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)**: Plan de implementación original

### Documentación Técnica (docs/)
- **[docs/STATUS.md](docs/STATUS.md)**: Estado actual del proyecto y opciones disponibles
- **[docs/ROADMAP.md](docs/ROADMAP.md)**: Hoja de ruta y análisis del proyecto
- **[docs/ROADMAP_DETAILS.md](docs/ROADMAP_DETAILS.md)**: Explicación técnica detallada de decisiones
- **[docs/PHASE2_WALKTHROUGH.md](docs/PHASE2_WALKTHROUGH.md)**: Walkthrough de implementación Fase 2

---

## 🐛 Solución de Problemas

### "Could not find nvcc"
```bash
# Compilar sin CUDA
cmake -DUSE_CUDA=OFF ..
```

### "CUDA OOM"
Usa modelos más pequeños o cuantizaciones más agresivas (q4_k_m en lugar de q5_k_m).

### "NVML initialization failed"
```bash
# Verificar drivers
nvidia-smi

# Reinstalar si es necesario
sudo apt-get install --reinstall nvidia-driver-535
```

Ver [BUILD.md](BUILD.md) para más detalles.

---

## 📄 Licencia

Este proyecto utiliza `llama.cpp` (MIT License). Consulta el repositorio original para más detalles.

---

## 🤝 Contribuciones

Este es un proyecto personal optimizado para hardware específico (GTX 1060 3GB). Si tienes sugerencias o mejoras, abre un issue.
