# Plan de Implementación: Inference-Core

Este documento detalla la arquitectura y pasos para construir un motor de inferencia de alto rendimiento, bajo consumo y alta estabilidad utilizando `llama.cpp` y C++.

## 1. Objetivos del Sistema
- **Rendimiento Máximo**: Utilizaremos acceso directo a la API de `llama.cpp` sin intermediarios (Python, etc.).
- **Estabilidad 24/7**: Gestión robusta de memoria, captura de excepciones, y un watchdog interno.
- **Optimización de Recursos**: Diseñado para una **GTX 1060 3GB**. Estrategia de "Split Computing" (CPU + GPU) inteligente.
- **Bajo Consumo**: Modo "Idle" cuando no hay peticiones, deteniendo el spinning de la GPU.

## 2. Arquitectura de Software

El proyecto se estructurará en componentes desacoplados:

```
src/
├── core/
│   ├── Engine.cpp/.h        # Lógica principal de llama.cpp (Contexto, Modelo, Sampling)
│   ├── Scheduler.cpp/.h     # Cola de peticiones (Thread-safe)
│   └── Allocator.cpp/.h     # Gestión de memoria pre-reservada (KV Cache)
├── server/
│   ├── WsServer.cpp/.h      # Servidor WebSocket de alto rendimiento (uWebSockets/WebSocket++)
│   ├── Protocol.h           # Definición de OPCODES y estructura de paquetes binarios/JSON
│   └── EventHandler.cpp     # Dispatcher de eventos (bidi-direccional)
├── hardware/
│   └── Monitor.cpp/.h       # Interfaz con NVML (Nvidia Management Library) para temperaturas/VRAM
└── main.cpp                 # Punto de entrada y gestión de señales (SIGINT, SIGTERM)
```

## 3. Estrategias de Bajo Nivel

### 3.1. Gestión de Memoria (GTX 1060 3GB)
Dado el límite de 3GB VRAM, implementaremos un cargador inteligente:
1.  **Detección de VRAM**: Al inicio, consultar VRAM libre.
2.  **Cálculo de Capas**: Calcular cuántas capas del modelo (quantizado q4_k_m o q5_k_m) caben en la VRAM disponible.
3.  **Offload Parcial**: Cargar el resto en RAM del sistema (CPU).
4.  **KV Cache Paginado**: Implementar un sistema de paginación para el KV Cache para evitar re-computar el prompt del sistema en cada petición si no ha cambiado.

### 3.2. Comunicación en Tiempo Real (WebSockets)
- **Protocolo de Eventos**: En lugar de HTTP Request/Response, usaremos un flujo continuo de eventos sobre TCP persistente.
    - **Client -> Server**: `{ "op": "GENERATE", "payload": { "prompt": "..." } }`
    - **Server -> Client**: `{ "op": "TOKEN", "token": " la", "prob": 0.98 }`
- **Zero-Latency Streaming**: Los tokens se envían al socket en cuanto salen del modelo, sin buffering.
- **Control Total**: El cliente puede enviar un evento `ABORT` para detener la generación instantáneamente y liberar recursos.

### 3.3. Estabilidad y Monitorización
- **Watchdog**: Un hilo secundario que verifica que el motor no se ha colgado (heartbeat).
- **Thermal Throttle**: Monitorizar temperatura vía NVML. Si pasa de 80°C, pausar/ralentizar inferencia para proteger el hardware.
- **Logging Estructurado**: Logs rotativos para diagnóstico post-mortem.

## 4. Stack Tecnológico
- **Lenguaje**: C++17 (o C++20 si el compilador lo soporta bien).
- **Core ML**: `llama.cpp` (linkado estático/directo).
- **Networking**: **uWebSockets** (uWS) o **WebSocket++** (Asio). Prioridad: Rendimiento y baja latencia.
- **JSON**: `nlohmann/json` (estándar de facto, robusto).
- **Monitorización**: `nvidia-ml` (NVML) para datos de GPU.

## 5. Roadmap de Desarrollo

### Fase 1: Core Engine (Cimientos) ✅
- [x] Compilación con CUDA.
- [ ] Clase `Engine` básica: Cargar modelo, tokenizar, evaluar.
- [ ] Implementar bucle de generación simple.

### Fase 2: Servidor WebSocket (Conectividad Real-Time)
- [ ] Integrar librería WebSocket de alto rendimiento.
- [ ] Definir protocolo de comunicación (Handshake, Heartbeat, Data). **IMPORTANTE**
- [ ] Implementar Streaming de tokens en tiempo real.
- [ ] Gestión de múltiples conexiones (Cola de espera si la GPU está ocupada).

### Fase 3: Optimización y Gestión (Robustez)
- [ ] Implementar descarga parcial inteligente (CPU/GPU split).
- [ ] Integrar NVML para monitorizar temperaturas.
- [ ] Pruebas de carga de larga duración (24h).

### Fase 4: Despliegue (Producción)
- [ ] Crear archivo `systemd` (.service).
- [ ] Script de auto-restauración en caso de fallo crítico.
