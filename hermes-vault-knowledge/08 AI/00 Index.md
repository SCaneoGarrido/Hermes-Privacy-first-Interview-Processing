---
title: 08 AI - Index
aliases: ["AI Index", "08 AI"]
tags: [moc, ai, index]
status: draft
created: 2026-07-24
updated: 2026-07-28
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["whisper.cpp - Repositorio Oficial", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-005 - Ollama como Motor LLM", "whisper.cpp Architecture", "ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio", "Ollama Integration Strategy", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama"]
---

# Summary

Índice de notas atómicas sobre los componentes de IA local de Hermes (transcripción y LLM). Sección pendiente de desarrollo.

# Explanation

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md` (alta prioridad):

- [x] whisper.cpp Architecture — ver [[whisper.cpp Architecture]] y [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]] (diseñado, implementado y verificado end-to-end 2026-07-28, ambos `status: stable`/`accepted`)
- [x] Ollama API — ver [[Ollama Integration Strategy]] y [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]] (diseño escrito 2026-07-28, `status: draft`/`proposed`, pendiente de aprobación antes de implementar Sprint 6)

Adicionalmente, pendiente documentar el modelo por defecto **Qwen** (.ai/PROJECT.md) como nota propia una vez se investigue su configuración concreta en Hermes.

Fuentes ya confirmadas el 2026-07-24 para desarrollar estos temas: [[whisper.cpp - Repositorio Oficial]] (que a su vez depende de [[ggml - Repositorio Oficial|ggml]]) y [[Ollama - Documentacion Oficial]] (endpoints de la API REST ya documentados: `/api/generate`, `/api/chat`, `/api/tags`, `/api/pull`, `/api/create`, `/api/embed`, `/api/delete`).

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
- [[ggml - Repositorio Oficial]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-005 - Ollama como Motor LLM]]
- [[Ollama - Documentacion Oficial]]
- [[whisper.cpp Architecture]]
- [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]
- [[Ollama Integration Strategy]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
- docs/documentation_tech.yml (Priority 2, categorías `whispercpp`, `ggml`, `ollama`)
