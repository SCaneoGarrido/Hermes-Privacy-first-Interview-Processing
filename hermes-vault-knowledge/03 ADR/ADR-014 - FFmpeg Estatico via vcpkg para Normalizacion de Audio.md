---
title: ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio
aliases: ["Audio Normalization", "libavcodec en Hermes"]
tags: [adr, hermes, libraries, ai]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: Diseño de sesión (Claude Code) + implementacion y verificacion end-to-end el mismo dia
related: ["ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-009 - libmariadb como Cliente MySQL", "whisper.cpp Architecture", "Filosofia de Repositorios", "Sprint 5 - Whisper Integration"]
---

# Context

Sprint 3 ya acepta audio en wav/ogg/m4a/mp3 en `POST /upload` (`AudioSignature.h` valida la firma de bytes de los cuatro formatos). whisper.cpp, en cambio, espera PCM 16-bit, 16kHz, mono — hace falta un paso de normalización antes de pasar el audio a `whisper_full` (ver [[whisper.cpp Architecture]]).

La opción obvia es FFmpeg, pero surgió una preocupación concreta al diseñar Sprint 5: si se usa como binario externo (`ffmpeg.exe` invocado vía `std::system`/subproceso), cualquier estudiante o colaborador que clone el repo tiene que instalarlo y configurarlo en `PATH` a mano antes de poder compilar y correr el backend. Hermes ya tiene un precedente de resolver exactamente este tipo de fricción: [[ADR-009 - libmariadb como Cliente MySQL]] eligió libmariadb sobre mysql-connector-cpp en parte porque compila sin pasos manuales bajo el toolchain de vcpkg + MinGW ya establecido (`x64-mingw-static`).

# Decision

Usar el puerto `ffmpeg` de vcpkg, con el mismo triplet estático que ya usa el resto del stack (`x64-mingw-static`), habilitando **solo** los componentes de decodificación (`avcodec`, `avformat`, `avutil`, `swresample`) — sin encoders, sin filtros de video, sin códecs GPL. Se linkea estático dentro de `backend.exe`, exactamente igual que Crow y libmariadb hoy.

Se envuelve detrás de una interfaz propia (ver [[Filosofia de Repositorios]]) — no se invoca como proceso externo en ningún punto del código.

# Alternatives

- **`ffmpeg.exe` como binario externo** (`std::system`/`popen`): descartado — exige instalación manual y `PATH` configurado por cada colaborador, rompe "clonar y compilar" sin pasos adicionales, y suma complejidad de manejo de subprocesos/errores en Windows que el resto del proyecto no tiene en ningún otro lado.
- **Restringir la entrada a solo WAV 16kHz mono, validado en la subida** (delegar la conversión al cliente): descartado por ahora — reabriría el contrato de Sprint 3 (`FileFormatGuard`/`AudioSignature`), movería la complejidad al frontend (fuera del alcance actual de [[Sprint 8 - Frontend]]) y degradaría la experiencia (el usuario tendría que convertir el archivo a mano antes de subirlo).
- **libsndfile vía vcpkg**: decodifica WAV/FLAC/Vorbis nativamente, pero no MP3 ni M4A/AAC sin códecs adicionales — no cubre los cuatro formatos que Sprint 3 ya acepta.

# Consequences

- `vcpkg.json` declara `ffmpeg` con `"default-features": false` y `"features": ["avcodec", "avformat", "swresample"]` (avutil viene incluido como dependencia base, no hace falta listarlo aparte). Confirmado en la práctica: compila limpio bajo `x64-mingw-static` sin intervención manual, ~17 minutos la primera vez (se cachea después).
- **Licencia**: con este feature set (decodificadores nativos de FFmpeg para PCM/MP3/Vorbis/AAC, sin códecs GPL como x264) el build cae bajo LGPL, compatible con distribuir el binario compilado. Reconfirmar en [[Sprint 12 - Release]] antes de empaquetar el instalador.
- El decodificador de FFmpeg es *stateless* por archivo — a diferencia de `whisper_context` (ver [[whisper.cpp Architecture]]), no necesita un mutex ni un ciclo de vida de singleton compartido entre workers. Confirmado: `FfmpegAudioNormalizer` no lleva estado propio.
- **Integración CMake real, distinta de lo asumido inicialmente**: el puerto de vcpkg expone FFmpeg via el módulo clásico `FindFFmpeg.cmake` (`find_package(FFMPEG REQUIRED)` + variables `FFMPEG_LIBRARIES`/`FFMPEG_INCLUDE_DIRS`), **no** targets modernos con namespace (`FFmpeg::avcodec`). El escaneo inicial de `vcpkg-cmake-wrapper.cmake` llevaba a pensar lo segundo; el error real de CMake (`target was not found`) lo corrigió.
- **Gotcha de plataforma descubierto en la práctica, no específico de FFmpeg pero encontrado acá**: con `CMAKE_BUILD_TYPE` vacío (default de un generador Ninja de un solo config si no se fija explícito), vcpkg resolvía `ggml-cpu.a` desde `vcpkg_installed/.../debug/lib/` mientras `libwhisper.a`/`ggml-base.a` salían de `.../lib/` (Release) — una mezcla Debug/Release ABI-incompatible entre paquetes del mismo build. El síntoma fue un crash silencioso (`STATUS_ACCESS_VIOLATION`, 0xC0000005) *dentro* de `whisper_full()`, sin ninguna excepción de C++ capturable, siempre en la misma dirección de memoria (determinístico). No reproducía en absoluto compilando a mano con `g++` apuntando solo a `.../lib/`. Fix: `CMakeLists.txt` ahora fija `CMAKE_BUILD_TYPE` a `Release` si no viene seteado, *antes* de `project()`. Ver [[whisper.cpp Architecture]], Common Mistakes.
- [[Stack Tecnologico de Hermes]] necesita una fila nueva para FFmpeg — pendiente, esa nota es `status: stable` y se actualiza aparte.

# Status

Accepted — implementado y verificado end-to-end (upload → normalizar → transcribir → resultado) el 2026-07-28. Ver [[Sprint 5 - Whisper Integration]].

# References

- Sesión de diseño 2026-07-28 (Priority 1, este documento)
- [[ADR-009 - libmariadb como Cliente MySQL]] (precedente del mismo criterio de decisión)
- https://github.com/microsoft/vcpkg/tree/master/ports/ffmpeg (Priority 4)
