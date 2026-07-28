---
title: whisper.cpp Architecture
aliases: ["Pipeline de Transcripcion", "Whisper Pipeline", "De Audio Mezclado a Dialogo Identificado"]
tags: [ai, hermes, whisper, architecture, sprint5]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: Diseño de sesión (Claude Code) + implementacion y verificacion end-to-end el mismo dia
related: ["ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio", "Sprint 4 - Background Processing", "Sprint 5 - Whisper Integration", "Sprint 6 - Ollama Integration", "Ollama Integration Strategy", "Filosofia de Repositorios", "whisper.cpp - Repositorio Oficial"]
---

# Summary

Pipeline de dos fases secuenciales para pasar de audio mezclado (un solo canal, investigador + entrevistado) a diálogo con actores identificados: whisper.cpp genera transcripción cruda con timestamps (Sprint 5), Ollama la convierte en diálogo etiquetado por actor razonando semánticamente sobre el texto — sin diarización real de audio (Sprint 6).

# Explanation

```
[Audio Mezclado] (Google Meet, grabación de celular, MP3, WAV)
       │
       ▼
 ┌────────────────────┐
 │ Normalizacion FFmpeg│ ➔ WAV PCM 16-bit, 16kHz, Mono (ver ADR-014)
 └────────────────────┘
       │
       ▼
 ┌──────────────────────────────────────────────────────┐
 │ FASE 1 (Sprint 5): whisper.cpp — Inferencia Local     │
 │  • Audio crudo de un solo canal.                      │
 │  • Segmenta por silencios/pausas.                      │
 │  • Genera JSON con marcas de tiempo por segmento.       │
 └──────────────────────────────────────────────────────┘
       │
       ▼
 [JSON intermedio con timestamps] (archivo en disco, ruta en interview_jobs)
       │
       ▼
 ┌──────────────────────────────────────────────────────┐
 │ FASE 2 (Sprint 6): Ollama — Analisis Semantico         │
 │  • Lee el JSON secuencial de la fase anterior.          │
 │  • Deduce quien es quien por contexto gramatical.        │
 │  • Reemplaza timestamps por tags de actor.                │
 └──────────────────────────────────────────────────────┘
       │
       ▼
[Resultado final] → interview_results, interviews.status = completed

Investigador: Hola, gracias por venir hoy. ¿Podrías presentarte?
Entrevistado: Claro, soy Carlos y trabajo como diseñador de software hace cinco años.
```

## Punto de enganche con Sprint 4

`InterviewProcessingJobHandler::execute(const Job&)` (hoy un stub con `sleep(2s)`, ver [[Sprint 4 - Background Processing]]) es el único lugar que cambia de contrato. `WorkerPool` no se toca: sigue llamando `execute()`, capturando excepciones y traduciéndolas a `markCompleted`/`markFailed` sobre `IInterviewJobRepository` exactamente igual que hoy.

Siguiendo [[Filosofia de Repositorios]], la inferencia queda detrás de una interfaz `ITranscriber` con implementación concreta `WhisperTranscriber`, inyectada en `InterviewProcessingJobHandler` desde el composition root (`main.cpp`), nunca instanciada dentro del handler.

## Fase 1 — Formato JSON intermedio

A través de `whisper_full_get_segment_t0`/`t1` (tiempo) y `whisper_full_get_segment_text` (texto decodificado), el handler arma un JSON intermedio por segmento:

```json
[
  {
    "start": "00:00:01",
    "end": "00:00:08",
    "text": " Hola, gracias por venir hoy. ¿Podrías presentarte?"
  },
  {
    "start": "00:00:09",
    "end": "00:00:15",
    "text": " Claro, soy Carlos y trabajo como diseñador de software hace cinco años."
  }
]
```

## Estrategia de almacenamiento (dos niveles)

**Sistema de archivos** (fuente de verdad del contenido, mismo criterio que ya usa el backend para audio/resultados):

```
storage/
  interviews/
    <interview_id>/
      audio.wav               (audio normalizado a 16kHz, salida de FFmpeg)
      transcript_raw.json     (salida exacta de whisper.cpp, Fase 1)
      transcript_final.txt    (diálogo formateado por Ollama, Fase 2)
```

**Base de datos** (solo metadata + ruta — nunca el contenido): `interviews_audio.interview_audio_path` e `interview_results.interview_transcription_file_path` ya siguen este patrón. `interview_jobs` (Sprint 4) gana una columna nueva del mismo tipo:

```sql
ALTER TABLE interview_jobs
    ADD COLUMN raw_transcript_path VARCHAR(500) NULL AFTER error_message;
```

**Desviación deliberada del diseño original al implementar**: NO se agregó el estado intermedio `transcribed` a `interview_jobs.status` — el rango se quedó en `pending | running | completed | failed` (igual que Sprint 4). Razón: sin Sprint 6 todavía implementado, nada consume ese estado intermedio; agregarlo ahora era complejidad prematura (ver [[Maintainability over Cleverness]]). En cambio, `raw_transcript_path` se guarda como metadata **sin cambiar el status** — `IInterviewJobRepository::saveRawTranscriptPath(interviewId, path)` hace un `UPDATE` puntual sobre el job `running`, nada más. Cuando Sprint 6 exista y de verdad necesite una pausa entre Fase 1 y Fase 2, ahí se justifica agregar el estado.

`interviews.status` se comporta tal cual se diseñó: se mantiene en `processing` durante toda la Fase 1 (no hay estados gruesos nuevos).

## Sprint 5 en solitario: resultado utilizable sin esperar a Ollama

El roadmap original de Sprint 5 (antes de este diseño) ya listaba "generación de TXT" como entregable propio, no solo JSON. Se mantuvo ese criterio: `InterviewProcessingJobHandler::execute()` no se detiene en el JSON crudo — genera además `transcript_final.txt` (concatenación simple de los segmentos, **sin diarizar**, sin tags de actor) y lo guarda vía el método nuevo `IInterviewRepository::upsertTranscriptionResult(interviewId, path)` (hasta este sprint, `interview_results` nunca se escribía en código — solo se leía). Al terminar, el job pasa a `completed` normal (contrato de `WorkerPool` sin cambios) y la entrevista queda con un resultado real y utilizable, aunque sin diarizar.

Cuando Sprint 6 (Ollama) exista, va a leer `raw_transcript_path` (el JSON con timestamps) y **reemplazar** el contenido de `transcript_final.txt` por la versión diarizada, vía el mismo `upsertTranscriptionResult` (ya es un UPSERT por diseño — soporta reescribir sin conflicto de `UNIQUE(interview_id)`). No hace falta un job type nuevo necesariamente; es una decisión a tomar en el diseño de Sprint 6.

## Conexión con Sprint 6

Cuando el Worker retome el job para la Fase 2, lee `raw_transcript_path` (o el archivo `transcript_raw.json`), y reescribe el resultado final vía `upsertTranscriptionResult` (ya resuelto en Sprint 5, ver arriba). **El diseño detallado de esta fase se movió a [[Ollama Integration Strategy]]** — el prompt de una sola pasada que se sugería acá originalmente no es viable con datos reales (ver nota siguiente).

**Confirmado con datos reales (interview_id=1, entrevista real de ~70 minutos, no un clip de prueba)**: `transcript_raw.json` generó **1804 segmentos**. Esto excede por mucho la ventana de contexto de un LLM local razonable en un solo prompt — el enfoque de "mandale todo el JSON a Ollama de una" queda descartado. Ver [[Ollama Integration Strategy]] para el diseño de chunking que lo reemplaza.

## Execution Time (decisión 2026-07-28)

`interview_jobs.started_at`/`finished_at` (Sprint 4) ya alcanzan para mostrar cuánto tardó un job. Se decidió **no** agregar una columna `execution_time_seconds` — se calcula al leer (`TIMESTAMPDIFF(SECOND, started_at, finished_at)`) y se expone en `GET /interview/:id`. Ventajas sobre una columna nueva: cero migración, funciona retroactivamente para jobs ya completados (interview_id=1 incluida, sin necesidad de reprocesar), y no duplica un dato ya derivable — coherente con el mismo criterio de "no vivan dos fuentes de verdad" ya aplicado a `raw_transcript_path` (ruta, no contenido) en este mismo sprint.

# Why it matters

Es la primera capacidad de IA local real del producto y la validación práctica de [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]. Sin este diseño resuelto, [[Sprint 6 - Ollama Integration]] no tiene de dónde leer datos.

# Best Practices

- `whisper_context` se carga **una sola vez**, de forma perezosa, en el primer `transcribe()` (no en el constructor ni en el composition root): así el backend arranca igual aunque el modelo `.bin` todavía no esté en disco (un researcher recién clonado el repo puede explorar el resto de la API sin haber descargado ~150MB). `ensureModelLoaded()` en `WhisperTranscriber` hace el chequeo de existencia del archivo y lanza un error claro y accionable si falta.
- El mismo mutex de `WhisperTranscriber` protege tanto la carga perezosa como `whisper_full()`: dos threads llamando `whisper_full` sobre el **mismo** `whisper_context` en paralelo no es seguro — misma clase de problema que ya se resolvió para `DatabaseManager` en Sprint 4. Implementado desde el día uno, no como mejora futura.
- La base de datos guarda rutas, nunca contenido — coherente con `interviews_audio`/`interview_results`. Evita tener dos fuentes de verdad (columna vs. archivo) que puedan divergir.
- Confirmado en la práctica: `whisper_full_get_segment_t0`/`t1` devuelven centisegundos (unidades de 10ms) en whisper.cpp 1.8.6, no milisegundos directos — el `*10` antes de convertir a `HH:MM:SS` es correcto y se verificó con audio real (timestamps `00:00:00`–`00:00:03` para un segmento de ~3s, coincide con la duración real).
- **Fijar `CMAKE_BUILD_TYPE` explícitamente en `CMakeLists.txt`** (no depender del default de un generador de un solo config) cuando el proyecto tenga dependencias vcpkg de múltiples paquetes vinculados entre sí (ej. `whisper-cpp` + `ggml`, donde `ggml-cpu` es un paquete separado de `ggml-base`). Ver Common Mistakes.
- **Sanitizar el texto de cada segmento antes de serializarlo**, no confiar en que `whisper_full_get_segment_text` devuelva siempre UTF-8 válido. `WhisperTranscriber::transcribe` corre `sanitizeUtf8()` sobre cada segmento apenas sale de whisper.cpp, y `writeRawTranscriptJson` además usa `nlohmann::json::error_handler_t::replace` como segunda red. Ver Common Mistakes.
- **Idioma explícito, no `"auto"`, cuando se conoce de antemano.** Hermes fija `"es"` por defecto (configurable por `WHISPER_LANGUAGE`) porque la mayoría de las entrevistas son en español: evita la pasada extra de detección y es más preciso que dejar que el modelo adivine, sobre todo en clips cortos/ruidosos o con modelos chicos.

# Common Mistakes

- Invocar `ffmpeg` como binario externo vía `std::system`/subproceso: rompe "clonar y compilar sin pasos manuales" para colaboradores nuevos. Ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]].
- Guardar el JSON crudo de whisper como `LONGTEXT` en `interview_jobs` en vez de como ruta a archivo — rompe el patrón ya establecido en el esquema y duplica la fuente de verdad.
- **Encontrado en producción, con audio real (entrevista de >100MB, no un clip de prueba sintético)**: whisper.cpp decodifica por token BPE, no por carácter — con audio largo/ruidoso, o con modelos chicos como `ggml-tiny`, puede emitir un token cuyos bytes truncan una secuencia UTF-8 multibyte a mitad de camino. `nlohmann::json::dump()` rechaza esos bytes con `type_error.316` ("invalid UTF-8 byte") y tumba el job entero — un fallo silencioso desde la perspectiva del usuario (la entrevista queda `failed` sin pista clara de la causa real en el mensaje corto). Los clips de prueba sintéticos (TTS, cortos, silenciosos) no lo disparan porque casi nunca generan tokens truncados; solo apareció con audio real largo. Fix: sanitizar antes de serializar (ver Best Practices), no solo capturar la excepción.
- Asumir que cada sub-fase interna (transcripción cruda vs. diarización) necesita su propio estado grueso en `interviews.status` — ese campo es deliberadamente grueso; el detalle vive en `interview_jobs.status`.
- **Encontrado en la práctica, costó horas de debugging**: dejar `CMAKE_BUILD_TYPE` vacío con el generador Ninja. vcpkg resolvió `ggml-cpu.a` desde la carpeta `debug/lib/` mientras `libwhisper.a`/`ggml-base.a` salían de `lib/` (Release) — sin ningún error de CMake ni del linker, compiló perfecto. El resultado fue un `whisper_full()` que crasheaba con `STATUS_ACCESS_VIOLATION` (0xC0000005) de forma silenciosa y determinística (misma dirección de memoria en cada corrida), sin excepción de C++ capturable — imposible de diagnosticar con `try`/`catch`. Se detectó recién comparando contra un `g++` manual (que sí funcionaba, porque apuntaba solo a `lib/`) y reproduciendo con un ejecutable mínimo que usaba las clases reales del proyecto sin Crow. Ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]], Consequences.

# Hermes Usage

Implementado y verificado end-to-end el 2026-07-28: `POST /interview/:id/process` con audio real (voz sintetizada, modelo `ggml-tiny.bin`) produjo `transcript_raw.json` y `transcript_final.txt` reales, `interviews.status = completed`, sin crashear en corridas repetidas. Extiende `InterviewProcessingJobHandler` (Sprint 4) sin tocar el contrato de `IJobQueue`/`WorkerPool`.

# Related Notes

- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]
- [[Sprint 4 - Background Processing]]
- [[Sprint 5 - Whisper Integration]]
- [[Sprint 6 - Ollama Integration]]
- [[Filosofia de Repositorios]]
- [[whisper.cpp - Repositorio Oficial]]

# References

- Sesión de diseño e implementación 2026-07-28 (Priority 1, este documento)
- https://github.com/ggml-org/whisper.cpp (Priority 4)
