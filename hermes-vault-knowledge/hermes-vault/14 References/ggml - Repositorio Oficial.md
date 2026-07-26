---
title: ggml - Repositorio Oficial
aliases: ["ggml"]
tags: [reference, ggml, ai]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/ggml-org/ggml
related: ["whisper.cpp - Repositorio Oficial", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "08 AI"]
---

# Summary

Repositorio oficial de ggml, la librería de tensores de bajo nivel sobre la que está construido whisper.cpp.

# Explanation

Fuente oficial: https://github.com/ggml-org/ggml

Verificado 2026-07-24: ggml es una librería de tensores de bajo nivel para machine learning, infraestructura base para inferencia y entrenamiento de modelos grandes. El propio repositorio indica que gran parte del desarrollo activo ocurre en los repos de `llama.cpp` y `whisper.cpp`, que se construyen sobre sus capacidades. Soporta diferenciación automática, algoritmos de optimización (ADAM, L-BFGS) y cuantización de enteros, evitando asignaciones de memoria en tiempo de ejecución.

# Why it matters

Es la capa de cómputo real que hace viable ejecutar whisper.cpp localmente sin GPU dedicada obligatoria (ver [[ADR-004 - whisper.cpp para Reconocimiento de Voz]] y el principio [[Local First]]): entender ggml es entender por qué whisper.cpp es eficiente en CPU.

# Best Practices

No tratar ggml como una dependencia directa de Hermes: Hermes depende de whisper.cpp, que a su vez depende de ggml. No acceder a la API de ggml directamente desde `ITranscriber`/`WhisperTranscriber`.

# Common Mistakes

Confundir ggml (librería de cómputo) con el formato de modelo `.bin` de ggml (formato de serialización de pesos); son conceptos relacionados pero distintos.

# Hermes Usage

Contexto técnico de soporte para la nota pendiente "whisper.cpp Architecture" en [[08 AI]].

# Related Notes

- [[whisper.cpp - Repositorio Oficial]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[08 AI]]

# References

- https://github.com/ggml-org/ggml (Priority 4, docs/documentation_tech.yml)
