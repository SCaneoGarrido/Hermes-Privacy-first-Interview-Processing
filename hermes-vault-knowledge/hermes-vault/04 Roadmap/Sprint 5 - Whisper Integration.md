---
title: Sprint 5 - Whisper Integration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 4 - Background Processing", "Sprint 6 - Ollama Integration", "ADR-004 - whisper.cpp para Reconocimiento de Voz"]
---

# Summary

Transcripción local de audio mediante whisper.cpp.

# Explanation

**Objetivo:** transcripción local.

**Entregables:** integración whisper.cpp, generación de TXT, generación de JSON, detección de idioma, configuración de modelo.

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

# References

- README.md (Priority 2)
