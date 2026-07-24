---
title: Sprint 10 - Testing
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 9 - Configuration", "Sprint 11 - Documentation"]
---

# Summary

Aseguramiento de calidad mediante pruebas unitarias, de integración, de estrés y benchmarks.

# Explanation

**Objetivo:** calidad.

**Entregables:** Unit Tests, Integration Tests, Stress Tests, Logging, Benchmarks.

# Why it matters

Valida que las decisiones tomadas en los sprints anteriores (especialmente el aislamiento de infraestructura vía interfaces) realmente permitan testear el dominio sin dependencias externas.

# Best Practices

- Usar Catch2 (ver [[Stack Tecnologico de Hermes]]) y dobles de prueba sobre las interfaces `ITranscriber`/`ILLMClient` en vez de instancias reales de whisper.cpp/Ollama en unit tests.

# Common Mistakes

- Escribir tests de integración que dependan de tener un modelo de whisper.cpp o de Ollama descargado, sin marcarlos como tales.

# Hermes Usage

Es prerequisito de calidad antes de [[Sprint 11 - Documentation]] y del release en [[Sprint 12 - Release]].

# Related Notes

- [[Sprint 9 - Configuration]]
- [[Sprint 11 - Documentation]]

# References

- README.md (Priority 2)
