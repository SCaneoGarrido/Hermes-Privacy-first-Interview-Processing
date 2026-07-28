---
title: Sprint 1 - Core API
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 0 - Foundation", "Sprint 2 - Persistence", "API First", "ADR-012 - Repository y Service Layer para Entrevistas", "ADR-013 - Versionado de API"]
---

# Summary

Construir el núcleo del backend: estructura modular, controllers, services, repositories y DTOs.

# Explanation

**Objetivo:** núcleo del backend.

**Entregables:** estructura modular, Controllers, Services, Repositories, [[DTO|DTOs]], configuración, manejo de errores, logging, versionado de API.

**Estado (2026-07-28):**
- ✅ Estructura modular, Controllers, manejo de errores, logging
- ✅ Services / Repositories — `InterviewService` / `IInterviewRepository` (ver [[ADR-012 - Repository y Service Layer para Entrevistas]]), agregados durante [[Sprint 2 - Persistence]] al pagar la deuda técnica, no en este sprint originalmente
- ✅ Versionado de API (`/api/v1`) — adoptado 2026-07-28, ver [[ADR-013 - Versionado de API]]
- ❌ DTOs como capa formal — los controllers construyen `crow::json::wvalue` directamente, sin clases DTO dedicadas
- ❌ Endpoint de Configuration — no existe todavía (depende de [[Sprint 9 - Configuration]])

**Endpoints:** Interview ✅, Configuration ❌ (pendiente), Health ✅.

# Why it matters

Define el esqueleto arquitectónico ([[Arquitectura en Capas de Hermes]]) que todos los sprints siguientes extenderán.

# Best Practices

- Establecer el versionado de API (`/api/v1/...`) desde este sprint, no después — no se cumplió en su momento, se adoptó recién en [[ADR-013 - Versionado de API]] (2026-07-28).
- Al cambiar el prefijo de rutas del backend, actualizar en el mismo cambio el cliente HTTP del frontend y el proxy de dev — no hacerlo rompió la app completa la primera vez (ver [[ADR-013 - Versionado de API]]).

# Common Mistakes

- Mezclar lógica de negocio en los controllers en vez de delegarla a los Services (.ai/AI_INSTRUCTIONS.md).

# Hermes Usage

Sienta las bases de [[API First]] y de la [[Filosofia de Repositorios]] para el resto del proyecto. Sigue sin cerrarse del todo: falta el endpoint de Configuration y una capa DTO formal.

# Related Notes

- [[Sprint 0 - Foundation]]
- [[Sprint 2 - Persistence]]
- [[Sprint 9 - Configuration]]
- [[API First]]
- [[ADR-012 - Repository y Service Layer para Entrevistas]]
- [[ADR-013 - Versionado de API]]

# References

- README.md (Priority 2)
