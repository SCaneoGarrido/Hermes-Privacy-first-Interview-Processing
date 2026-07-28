---
title: MOC - Arquitectura
aliases: ["Architecture MOC", "02 Architecture", "03 ADR"]
tags: [moc, architecture]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: 
related: ["Home"]
---

# Summary

Mapa de contenido de las secciones [[02 Architecture]] y [[03 ADR]]: la arquitectura de Hermes y las decisiones que la sostienen (ver también el concepto de [[ADR]] en el glosario).

# Explanation

**Conceptos de arquitectura**
- [[Arquitectura en Capas de Hermes]]
- [[Clean Architecture]]
- [[Modular Monolith]]

**Decisiones (ADR)**
- [[ADR-001 - Modular Monolith]]
- [[ADR-002 - C++20 como Lenguaje]]
- [[ADR-003 - Crow como Framework REST]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-006 - SQLite como Base de Datos]] (superseded)
- [[ADR-007 - Prohibicion de Cloud por Defecto]]
- [[ADR-008 - MySQL como Base de Datos]]
- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[ADR-010 - Docker Compose para MySQL Local]]
- [[ADR-011 - Contrato de Respuesta API Uniforme]]

**Patrones relacionados**
- [[Filosofia de Repositorios]] (01 Project) — **desviación conocida (2026-07-28)**: los controllers acceden a `DatabaseManager` directamente, sin repositorio intermedio (ver [[Sprint 2 - Persistence]])
- [[09 Patterns]]: [[Contrato de Respuesta API Uniforme]] (disponible); Dependency Injection, Repository Pattern, DTO Pattern (pendientes)

# Why it matters

Conecta el "qué" (arquitectura en capas) con el "por qué" (ADRs), evitando que las decisiones queden implícitas en el código.

# Best Practices

Cada vez que se acepte una nueva ADR, añadir su enlace aquí.

# Common Mistakes

N/A — nota de organización.

# Hermes Usage

N/A — nota de organización.

# Related Notes

- [[Home]]
- [[MOC - Proyecto]]

# References

(nota de organización, sin fuente externa)
