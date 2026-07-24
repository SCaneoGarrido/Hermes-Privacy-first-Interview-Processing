---
title: Sprint 1 - Core API
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 0 - Foundation", "Sprint 2 - Persistence", "API First"]
---

# Summary

Construir el núcleo del backend: estructura modular, controllers, services, repositories y DTOs.

# Explanation

**Objetivo:** núcleo del backend.

**Entregables:** estructura modular, Controllers, Services, Repositories, [[DTO|DTOs]], configuración, manejo de errores, logging, versionado de API.

**Endpoints:** Interview, Configuration, Health.

# Why it matters

Define el esqueleto arquitectónico ([[Arquitectura en Capas de Hermes]]) que todos los sprints siguientes extenderán.

# Best Practices

- Establecer el versionado de API (`/api/v1/...`) desde este sprint, no después.

# Common Mistakes

- Mezclar lógica de negocio en los controllers en vez de delegarla a los Services (.ai/AI_INSTRUCTIONS.md).

# Hermes Usage

Sienta las bases de [[API First]] y de la [[Filosofia de Repositorios]] para el resto del proyecto.

# Related Notes

- [[Sprint 0 - Foundation]]
- [[Sprint 2 - Persistence]]
- [[API First]]

# References

- README.md (Priority 2)
