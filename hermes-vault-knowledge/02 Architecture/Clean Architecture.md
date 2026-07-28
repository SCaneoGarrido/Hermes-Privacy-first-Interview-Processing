---
title: Clean Architecture
aliases: []
tags: [architecture, pattern, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: .ai/PROJECT.md
related: ["Arquitectura en Capas de Hermes", "Filosofia de Repositorios", "Dependency Injection", "Repository Pattern", "ADR-008 - MySQL como Base de Datos"]
---

# Summary

Principio arquitectónico de Hermes por el cual los frameworks son detalles de implementación y la lógica de negocio no depende de ellos.

# Explanation

En Hermes, Clean Architecture se traduce en una regla concreta: "Frameworks are implementation details. Business logic must never depend on Crow, MySQL, Whisper or Ollama." Esto se implementa mediante la [[Arquitectura en Capas de Hermes]] y la [[Filosofia de Repositorios]] (interfaces como `ITranscriber`, `ILLMClient`).

Este concepto proviene originalmente del libro *Clean Architecture: A Craftsman's Guide to Software Structure and Design* de Robert C. Martin (ver docs/Libros), fuente de aprendizaje aprobada para el proyecto (Priority 3).

# Why it matters

Permite que Hermes evolucione su stack tecnológico (por ejemplo, cambiar whisper.cpp por otro motor de ASR) sin reescribir la lógica de dominio, y facilita las pruebas unitarias al aislar el dominio de dependencias externas.

# Best Practices

- El dominio no debe conocer tipos de Crow, MySQL ni de los SDKs de whisper.cpp/Ollama.
- Las conversiones entre entidades de dominio y modelos de infraestructura ocurren en los adaptadores de `Infrastructure`.

# Common Mistakes

- Filtrar tipos de una librería de infraestructura (por ejemplo, un `MYSQL_STMT*`) hasta la capa de `Domain`.

# Hermes Usage

Ver [[Arquitectura en Capas de Hermes]] para el mapeo concreto de capas en Hermes, y [[03 ADR]] para las decisiones tecnológicas que esta regla protege.

# Related Notes

- [[Arquitectura en Capas de Hermes]]
- [[Filosofia de Repositorios]]
- [[Dependency Injection]]
- [[Repository Pattern]]

# References

- .ai/PROJECT.md (Priority 1)
- Robert C. Martin, *Clean Architecture: A Craftsman's Guide to Software Structure and Design* (Priority 3, docs/Libros)
