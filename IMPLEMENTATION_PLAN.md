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
│   ├── WsServer.cpp/.h      # Wrapper sobre uWebSockets
│   ├── Client.h             # Estructura de estado por cliente
│   └── Protocol.h           # Constantes y Parsers JSON
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
- **Lenguaje**: C++17.
- **Core ML**: `llama.cpp`.
- **Networking**: `uWebSockets` (uWS) + `uSockets`. 
    - **Motivo**: Arquitectura basada en eventos (libuv/epoll), uso de memoria mínimo y rendimiento superior a Asio.
- **JSON**: `nlohmann/json`.
- **Monitorización**: `nvidia-ml` (NVML) para datos de GPU.

## 5. Roadmap de Desarrollo

### Fase 1: Core Engine (Cimientos) ✅
- [x] Compilación con CUDA.
- [ ] Clase `Engine` básica: Cargar modelo, tokenizar, evaluar.
- [ ] Implementar bucle de generación simple.

### Fase 2: Servidor WebSocket (Conectividad Real-Time)
#### 2.1 Dependencias
- [ ] Configurar `FetchContent` en CMake para descargar `uWebSockets` y `uSockets`.
- [ ] Configurar `nlohmann/json`.

#### 2.2 Protocolo (JSON sobre WS)
**Client -> Server**
- `INFER`: `{ "op": "infer", "prompt": "...", "params": { "temp": 0.7 } }`
- `ABORT`: `{ "op": "abort" }`

**Server -> Client**
- `HELLO`: `{ "op": "hello", "status": "ready", "model": "..." }`
- `TOKEN`: `{ "op": "token", "content": " The" }`
- `END`:   `{ "op": "end", "stats": { "ms": 1200, "tps": 45.2 } }`
- `ERROR`: `{ "op": "error", "message": "OOM" }`

#### 2.3 Implementación
- [ ] Crear clase `WsServer` (Singleton o gestor principal).
- [ ] Loop de eventos en hilo principal.
- [ ] Cola de tareas: El WebSocket corre en el Hilo A, el `Engine` en el Hilo B.
- [ ] Implementar métricas: TPS (Tokens/sec), Latencia TTFT (Time To First Token), Tiempo total.
    - Cuando llega `INFER`, se encola.
    - El Hilo B procesa y llama al callback.
    - El callback (desde Hilo B) debe notificar al Hilo A para enviar el mensaje (uWS no es thread-safe para envíos directos desde otros hilos, requiere `loop->defer`).

### Fase 3: Optimización y Gestión (Robustez)
- [ ] Implementar descarga parcial inteligente (CPU/GPU split).
- [ ] Integrar NVML para monitorizar temperaturas.
- [ ] Pruebas de carga de larga duración (24h).

### Fase 4: Despliegue (Producción)
- [ ] Crear archivo `systemd` (.service).
- [ ] Script de auto-restauración en caso de fallo crítico.
