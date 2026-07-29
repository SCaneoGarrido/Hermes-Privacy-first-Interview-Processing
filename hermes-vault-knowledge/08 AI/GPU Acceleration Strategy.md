---
title: GPU Acceleration Strategy
aliases: ["Aceleracion GPU", "Vulkan whisper.cpp", "HSA_OVERRIDE_GFX_VERSION"]
tags: [ai, hermes, gpu, architecture, sprint5, sprint6, backlog]
status: draft
created: 2026-07-29
updated: 2026-07-29
source: Diseño de sesión (Claude Code) — pendiente de implementación, documentado antes de cerrar sesión a pedido explícito
related: ["whisper.cpp Architecture", "Ollama Integration Strategy", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-005 - Ollama como Motor LLM"]
---

# Summary

Plan para aprovechar GPU cuando esté disponible, tanto en whisper.cpp (in-process, control total) como en Ollama (proceso externo, control limitado a detectar y aconsejar). **No implementado todavía** — Sprint 5/6 corren 100% CPU hoy (`use_gpu = false` fijo en `WhisperTranscriber`, sin features de GPU en el `ggml`/`whisper-cpp` de vcpkg).

# Explanation

## El camino es distinto para whisper.cpp y para Ollama — no es lo mismo

**whisper.cpp / ggml (in-process, Hermes lo controla por completo)**

El puerto de vcpkg de `ggml` (0.11.1) expone estos features de GPU (confirmado leyendo `ggml/vcpkg.json` completo, no una suposición):

| Feature | Vendor | Soportado en `x64-mingw-static`? |
|---|---|---|
| `cuda` | NVIDIA | **No** — `"supports": "!(windows & staticcrt)"`, nuestro triplet es justamente windows+staticcrt |
| `metal` | Apple | No — `"supports": "osx"` |
| `opencl` | Generico | Si (`"supports": "!arm32"`) |
| `vulkan` | Generico (AMD/NVIDIA/Intel) | Si, sin restriccion de plataforma declarada |

Para una AMD (ej. RX 9060 XT del researcher), **Vulkan es el camino real y vcpkg-buildable** en este toolchain. `opencl` es la alternativa si Vulkan da problemas en la práctica, pero suele tener peor cobertura de operadores en ggml. **No hace falta `HSA_OVERRIDE_GFX_VERSION` para whisper.cpp** — esa variable es especifica de ROCm/HIP, que ni siquiera está expuesto como feature en este puerto de vcpkg (implementar soporte ROCm/HIP real requeriría compilar ggml por fuera de vcpkg, un esfuerzo bastante mayor, no contemplado acá).

**Plan concreto para whisper.cpp:**
1. Agregar el feature `vulkan` a `ggml`/`whisper-cpp` en `vcpkg.json`.
2. En `WhisperTranscriber::ensureModelLoaded()`, detectar en runtime si hay un dispositivo Vulkan compatible disponible (whisper.cpp expone la info de backends registrados; alternativa: consultar la API de Vulkan directo con `vkEnumeratePhysicalDevices` antes de inicializar el contexto).
3. Setear `whisper_context_params.use_gpu = true` solo si se detecto un dispositivo — si no, cae a CPU automaticamente, mismo criterio de degradacion con gracia que ya se uso para el modelo faltante (ver [[whisper.cpp Architecture]]).
4. Validar que `flash_attn` siga en `false` o probarlo de nuevo con GPU — el crash de Sprint 5 (ver whisper.cpp Architecture, Common Mistakes) fue especificamente en la ruta CPU; no esta confirmado si la ruta GPU tiene el mismo problema.

**Ollama (proceso externo, Hermes NO lo lanza ni administra — ADR-005)**

Ollama corre como servicio independiente, ya arriba, al que `OllamaClient` le habla por HTTP (`localhost:11434`). Las variables de entorno de un proceso solo se fijan al lanzarlo — Hermes no puede modificarlas en un proceso que ya esta corriendo. Ollama en AMD usa ROCm internamente, que sí sufre el problema de "gfx version no soportada" en muchas GPUs de consumo recientes (RDNA3/RDNA4), de ahi que `HSA_OVERRIDE_GFX_VERSION` (ej. `11.0.0` para RDNA3 en el caso real del researcher) sea necesario para que Ollama la reconozca.

**Decisión de alcance (2026-07-29, confirmada por el owner del proyecto)**: Hermes **no** administra el ciclo de vida de Ollama. Se evaluaron dos opciones:
- ~~Hermes lanza/reinicia Ollama con las variables correctas~~ — descartado: cambia la arquitectura (Hermes pasaria a gestionar un proceso externo: puertos ocupados, reinicios, comportamiento distinto por SO), no es lo que ADR-005 asumió originalmente, complejidad que el proyecto no necesita todavia.
- **Hermes detecta la GPU disponible y aconseja** (elegido) — al arrancar, loguea/muestra el comando exacto sugerido segun el hardware detectado, sin tocar el proceso de Ollama. Mismo espiritu que `WHISPER_MODEL_PATH`/`OLLAMA_MODEL`: el researcher administra sus propios servicios externos, Hermes lo guia pero no lo hace por él.

**Plan concreto para el aviso de Ollama:**
1. Detectar vendor/modelo de GPU disponible (Windows: se puede consultar via WMI/DXGI el nombre del adaptador; necesita investigacion de la API concreta a usar desde C++ sin agregar una dependencia pesada).
2. Si es AMD: loguear al arrancar algo como *"GPU AMD detectada (<modelo>). Si Ollama no la esta usando, probablemente necesites HSA_OVERRIDE_GFX_VERSION -- ver https://github.com/ollama/ollama para el valor correspondiente a tu arquitectura, y reiniciar el servicio de Ollama con esa variable seteada."*
3. Si es NVIDIA: Ollama suele detectar CUDA sin pasos extra -- no hace falta el mismo aviso, aunque vale la pena confirmarlo en la práctica antes de asumirlo.
4. Sin GPU detectada: no imprimir nada, o un log informativo de que corre 100% CPU (ya es el comportamiento actual).

## Por qué esto no se implementó ya

Se documentó a pedido explícito antes de cerrar la sesión de trabajo, en vez de implementarse de una: after Sprint 5/6 (una maratón real de bugs de build + hallazgos de calidad), la prioridad es dejar esto planificado con la investigación ya hecha (evita repetir el error de "asumir sin chequear el vcpkg.json real" que costó tiempo real con FFmpeg/httplib en sprints anteriores) para retomarlo con una sesión dedicada.

# Why it matters

Whisper y Ollama corriendo en CPU son el cuello de botella mas grande de rendimiento del pipeline completo (~23 min para una entrevista de 70 min en la corrida real, ver [[Ollama Integration Strategy]]). GPU es la palanca de mayor impacto para reducir ese tiempo, mayor que cualquier ajuste de chunking o modelo mas chico.

# Best Practices

- Confirmar el feature `vulkan` de vcpkg realmente compila antes de asumir que funciona -- mismo criterio que con todo lo demas este sprint, no declarar nada "listo" sin haberlo corrido de verdad.
- Degradacion con gracia obligatoria: sin GPU compatible, todo tiene que seguir funcionando en CPU exactamente como hoy, sin excepciones ni fallos silenciosos.

# Common Mistakes

- Asumir que `HSA_OVERRIDE_GFX_VERSION` aplica a whisper.cpp -- es especifico de Ollama/ROCm, whisper.cpp con Vulkan no lo necesita.
- Asumir que Hermes puede controlar las variables de entorno de Ollama en runtime -- es un proceso externo ya corriendo, eso solo funciona si Hermes lo lanza (descartado, ver Decision de alcance arriba).

# Hermes Usage

Backlog de trabajo pendiente para retomar en una sesión dedicada, no bloqueante para lo ya implementado (Sprint 5/6 siguen funcionando 100% en CPU). Ver también el backlog de calidad de Sprint 6 en [[Ollama Integration Strategy]] -- ambos quedaron documentados el mismo día, pendientes de priorizar cuál se ataca primero.

# Related Notes

- [[whisper.cpp Architecture]]
- [[Ollama Integration Strategy]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-005 - Ollama como Motor LLM]]

# References

- Sesión de diseño 2026-07-29 (Priority 1, este documento — pendiente de implementación)
- `E:\Desarrollo\Tools\vcpkg\ports\ggml\vcpkg.json` (Priority 4, leído completo el 2026-07-29 para confirmar features de GPU disponibles)
- https://github.com/ollama/ollama (Priority 4 — variables de entorno de GPU para AMD/ROCm)
