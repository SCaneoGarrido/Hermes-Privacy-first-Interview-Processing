---
title: 08 AI - Index
aliases: ["AI Index", "08 AI"]
tags: [moc, ai, index]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["whisper.cpp - Repositorio Oficial", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-005 - Ollama como Motor LLM"]
---

# Summary

Índice de notas atómicas sobre los componentes de IA local de Hermes (transcripción y LLM). Sección pendiente de desarrollo.

# Explanation

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md` (alta prioridad):

- [ ] whisper.cpp Architecture
- [ ] Ollama API

Adicionalmente, pendiente documentar el modelo por defecto **Qwen** (.ai/PROJECT.md) como nota propia una vez se investigue su configuración concreta en Hermes.

# Why it matters

Estas dos tecnologías son el núcleo de la propuesta de valor de Hermes: IA de calidad ejecutándose 100% local, sustentando [[Privacy First]] y [[Local First]].

# Best Practices

Documentar la arquitectura de whisper.cpp y la API de Ollama en el nivel de detalle necesario para justificar decisiones de integración (por ejemplo, streaming de resultados, gestión de modelos locales).

# Common Mistakes

Mezclar en esta sección detalles de infraestructura genérica (SQLite, Crow): esta sección es específica de IA/ML.

# Hermes Usage

Ver [[ADR-004 - whisper.cpp para Reconocimiento de Voz]], [[ADR-005 - Ollama como Motor LLM]], [[Sprint 5 - Whisper Integration]] y [[Sprint 6 - Ollama Integration]].

# Related Notes

- [[whisper.cpp - Repositorio Oficial]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-005 - Ollama como Motor LLM]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
