---
title: whisper.cpp - Repositorio Oficial
aliases: []
tags: [reference, whisper.cpp, ai]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/ggml-org/whisper.cpp
related: ["ADR-004 - whisper.cpp para Reconocimiento de Voz"]
---

# Summary

Repositorio oficial de whisper.cpp, el motor de transcripción local de Hermes.

# Explanation

Fuente oficial: https://github.com/ggml-org/whisper.cpp

Implementación en C/C++ del modelo Whisper de reconocimiento de voz, sin dependencias de Python en tiempo de ejecución.

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

# References

- https://github.com/ggml-org/whisper.cpp (Priority 4)
