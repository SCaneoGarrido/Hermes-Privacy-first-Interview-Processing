---
title: whisper.cpp - Repositorio Oficial
aliases: []
tags: [reference, whisper.cpp, ai]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/ggml-org/whisper.cpp
related: ["ADR-004 - whisper.cpp para Reconocimiento de Voz", "ggml - Repositorio Oficial"]
---

# Summary

Repositorio oficial de whisper.cpp, el motor de transcripción local de Hermes.

# Explanation

Fuentes oficiales:
- https://github.com/ggml-org/whisper.cpp
- https://github.com/ggml-org/whisper.cpp/tree/master/examples (ejemplos de uso, incluye bindings)

Implementación en C/C++ de inferencia del modelo Whisper de OpenAI para reconocimiento de voz, sin dependencias de Python en tiempo de ejecución. La implementación de alto nivel vive en `whisper.h`/`whisper.cpp`, mientras que el cómputo de bajo nivel lo resuelve la librería [[ggml - Repositorio Oficial|ggml]]. Soporta macOS, iOS, Android, Linux, Windows, WebAssembly y Raspberry Pi, con bindings para Rust, JavaScript, Go, Python y otros. Los modelos se cargan desde un formato binario propio de ggml (descargable con scripts incluidos) y se ejecutan vía una API estilo C, con soporte de CPU y varios aceleradores GPU.

# Why it matters

Es la tecnología que hace posible transcribir localmente sin enviar audio a servicios cloud, pieza central de [[ADR-004 - whisper.cpp para Reconocimiento de Voz]].

# Best Practices

Verificar la licencia y el tamaño de los modelos antes de distribuirlos junto al instalador (ver [[Sprint 12 - Release]]).

# Common Mistakes

N/A — nota de referencia.

# Hermes Usage

Ver pendiente: nota atómica "whisper.cpp Architecture" en [[08 AI]] (Knowledge Backlog, alta prioridad).

# Related Notes

- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ggml - Repositorio Oficial]]

# References

- https://github.com/ggml-org/whisper.cpp (Priority 4)
- https://github.com/ggml-org/whisper.cpp/tree/master/examples (Priority 4, docs/documentation_tech.yml)
