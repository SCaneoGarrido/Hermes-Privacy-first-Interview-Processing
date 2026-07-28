---
title: Arquitectura en Capas de Hermes
aliases: ["Hermes Layered Architecture"]
tags: [architecture, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: .ai/ARCHITECTURE.md
related: ["Clean Architecture", "Modular Monolith", "API First", "Filosofia de Repositorios", "ADR-008 - MySQL como Base de Datos"]
---

# Summary

Hermes organiza su flujo de ejecución en cinco capas estrictamente direccionadas: React → REST API → Application → Domain → Infrastructure.

# Explanation

```
React
  ↓
REST API
  ↓
Application
  ↓
Domain
  ↓
Infrastructure
  ↓
whisper.cpp · MySQL · Ollama
```

Cada flecha representa una dependencia permitida en una única dirección: una capa superior puede depender de una inferior, nunca al revés. `Infrastructure` es la única capa que conoce las tecnologías concretas (whisper.cpp, MySQL, Ollama); `Domain` y `Application` las desconocen por completo y solo ven interfaces (ver [[Filosofia de Repositorios]]).

# Why it matters

Esta dirección estricta de dependencias es lo que hace posible la regla de [[Clean Architecture]] de Hermes: "Business logic must never depend on Crow, MySQL, Whisper or Ollama." Sin esta disciplina, un cambio en whisper.cpp obligaría a modificar el dominio.

# Best Practices

- Nunca importar un header de Crow, MySQL o whisper.cpp desde `Domain` o `Application`.
- Los DTOs cruzan de `REST API` a `Application`; las entidades de dominio no deberían serializarse directamente.

# Common Mistakes

- Colocar validaciones de negocio en los controllers de la capa REST API (.ai/AI_INSTRUCTIONS.md: "Never introduce business logic inside controllers").
- Acceder a la base de datos directamente desde un controller, saltándose `Application` y `Domain`. **Corregido (2026-07-28)**: `InterviewController`/`FileController` dependen de `InterviewService` (capa `Application`), que a su vez depende de `IInterviewRepository` — ver [[ADR-012 - Repository y Service Layer para Entrevistas]] y [[Sprint 2 - Persistence]].

# Hermes Usage

Esta es la arquitectura de referencia para todo el backend de Hermes; toda nueva funcionalidad debe ubicarse explícitamente en una de estas cinco capas.

# Related Notes

- [[Clean Architecture]]
- [[Modular Monolith]]
- [[API First]]
- [[Filosofia de Repositorios]]

# References

- .ai/ARCHITECTURE.md (Priority 1)
