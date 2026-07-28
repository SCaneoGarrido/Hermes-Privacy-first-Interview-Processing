---
title: 09 Patterns - Index
aliases: ["Patterns Index", "09 Patterns"]
tags: [moc, patterns, index]
status: draft
created: 2026-07-24
updated: 2026-07-28
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["Filosofia de Repositorios", "Clean Architecture", "Contrato de Respuesta API Uniforme"]
---

# Summary

Índice de patrones de diseño de software relevantes para Hermes.

# Explanation

**Disponible**
- [x] [[Contrato de Respuesta API Uniforme]] — envelope `{success, data, error}` uniforme en toda la REST API (ver [[ADR-011 - Contrato de Respuesta API Uniforme]])

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md` (prioridad media), todavía pendientes:

- [ ] Dependency Injection
- [ ] Repository Pattern
- [ ] DTO Pattern (ver definición corta en [[DTO]])

Estos tres patrones ya están descritos conceptualmente (sin nota atómica propia todavía) en [[Filosofia de Repositorios]] y en la [[Arquitectura en Capas de Hermes]].

# Why it matters

Son los patrones que hacen operativa la [[Clean Architecture]] de Hermes: DI para componer implementaciones concretas, Repository para ocultar SQLite, DTO para cruzar la frontera REST API ↔ Application sin filtrar entidades de dominio.

# Best Practices

Cada nota debe incluir un ejemplo concreto tomado de Hermes (`ITranscriber`/`WhisperTranscriber`, `ILLMClient`/`OllamaClient`).

# Common Mistakes

Escribir estas notas de forma genérica sin conectarlas al código real de Hermes (contradice "Avoid generic explanations disconnected from the project").

# Hermes Usage

Ver [[Filosofia de Repositorios]] para el caso de uso completo.

# Related Notes

- [[Filosofia de Repositorios]]
- [[Clean Architecture]]
- [[Contrato de Respuesta API Uniforme]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
