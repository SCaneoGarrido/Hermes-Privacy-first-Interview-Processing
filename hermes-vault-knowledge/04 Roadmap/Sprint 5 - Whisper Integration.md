---
title: Sprint 5 - Whisper Integration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 4 - Background Processing", "Sprint 6 - Ollama Integration", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "whisper.cpp Architecture", "ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio"]
---

# Summary

Transcripción local de audio mediante whisper.cpp.

# Explanation

**Objetivo:** transcripción local.

**Entregables:** integración whisper.cpp, generación de JSON intermedio con timestamps por segmento (Fase 1 del pipeline, ver [[whisper.cpp Architecture]]), normalización de audio de entrada vía FFmpeg estático (ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]), detección de idioma, configuración de modelo.

> **Implementado y verificado end-to-end el 2026-07-28** — ver [[whisper.cpp Architecture]] y [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]] (ambos `status: stable`/`accepted`). Sprint 5 entrega JSON crudo con timestamps **y** un TXT final utilizable (sin diarizar) — no se esperó a Sprint 6 para tener un resultado real. La diarización por actor (`Investigador:`/`Entrevistado:`) queda para [[Sprint 6 - Ollama Integration]], que va a reemplazar el contenido del TXT.

# Why it matters

Es la primera capacidad de IA local del producto y la validación práctica de [[ADR-004 - whisper.cpp para Reconocimiento de Voz]].

# Best Practices

- Exponer whisper.cpp únicamente a través de la interfaz `ITranscriber` (ver [[Filosofia de Repositorios]]).

# Common Mistakes

- Acoplar el formato de salida de whisper.cpp directamente a los DTOs de la API sin una capa de traducción.

# Hermes Usage

Habilita el pipeline de anonimización y resumen de [[Sprint 6 - Ollama Integration]].

# Related Notes

- [[Sprint 4 - Background Processing]]
- [[Sprint 6 - Ollama Integration]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[whisper.cpp Architecture]]
- [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]

# References

- README.md (Priority 2)
