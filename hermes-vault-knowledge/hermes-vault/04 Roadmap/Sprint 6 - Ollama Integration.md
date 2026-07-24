---
title: Sprint 6 - Ollama Integration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 5 - Whisper Integration", "Sprint 7 - Export", "ADR-005 - Ollama como Motor LLM"]
---

# Summary

Integración de IA local (Ollama) para corrección ortográfica, puntuación, resúmenes y anonimización.

# Explanation

**Objetivo:** integración IA local.

**Entregables:** cliente HTTP hacia Ollama, corrección ortográfica, puntuación, resúmenes, anonimización.

# Why it matters

La anonimización es el entregable más directamente ligado al principio [[Privacy First]]: convierte una transcripción cruda en un documento seguro de compartir.

# Best Practices

- Aislar el cliente de Ollama detrás de `ILLMClient` (ver [[Filosofia de Repositorios]]) para poder cambiar de modelo sin tocar el dominio.

# Common Mistakes

- Asumir que la anonimización del LLM es 100% infalible sin validación o revisión adicional antes de exportar.

# Hermes Usage

Cierra el pipeline de procesamiento de entrevistas antes de la exportación en [[Sprint 7 - Export]].

# Related Notes

- [[Sprint 5 - Whisper Integration]]
- [[Sprint 7 - Export]]
- [[ADR-005 - Ollama como Motor LLM]]

# References

- README.md (Priority 2)
