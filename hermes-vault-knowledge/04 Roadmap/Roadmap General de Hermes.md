---
title: Roadmap General de Hermes
aliases: ["Hermes Roadmap"]
tags: [roadmap, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Hermes - Vision General"]
---

# Summary

Hermes se desarrolla en 13 sprints (Sprint 0 a Sprint 12), desde la configuración del entorno hasta la primera versión estable 1.0.

# Explanation

> **Nota de fuente:** `.ai/ROADMAP.md` existe pero está vacío al momento de esta nota (2026-07-24). El contenido del roadmap se ha tomado de `README.md` (Priority 2 / documentación complementaria del repositorio), no de `.ai/`. Cuando `.ai/ROADMAP.md` se complete, esta nota y las notas de sprint deben revisarse y reconciliarse con esa fuente de Priority 1.

Secuencia de sprints:

1. [[Sprint 0 - Foundation]]
2. [[Sprint 1 - Core API]]
3. [[Sprint 2 - Persistence]]
4. [[Sprint 3 - File Upload]]
5. [[Sprint 4 - Background Processing]]
6. [[Sprint 5 - Whisper Integration]]
7. [[Sprint 6 - Ollama Integration]]
8. [[Sprint 7 - Export]]
9. [[Sprint 8 - Frontend]]
10. [[Sprint 9 - Configuration]]
11. [[Sprint 10 - Testing]]
12. [[Sprint 11 - Documentation]]
13. [[Sprint 12 - Release]]

# Status (2026-07-28)

| # | Sprint | Estado | Nota |
|---|--------|--------|------|
| 0 | [[Sprint 0 - Foundation]] | ✅ done | Entorno, CMake/vcpkg, Crow, MySQL, logging, `/health` |
| 1 | [[Sprint 1 - Core API]] | 🟠 in-progress | Services/Repositories y versionado `/api/v1` ya resueltos ([[ADR-012 - Repository y Service Layer para Entrevistas]], [[ADR-013 - Versionado de API]]); falta endpoint Configuration y DTOs formales |
| 2 | [[Sprint 2 - Persistence]] | ✅ done | CRUD completo (incl. `DELETE`) + `IInterviewRepository`/`InterviewService` ([[ADR-012 - Repository y Service Layer para Entrevistas]]) |
| 3 | [[Sprint 3 - File Upload]] | 🟠 in-progress | Upload + validación de firma de bytes; falta progreso de subida (depende de Sprint 4) |
| 4 | [[Sprint 4 - Background Processing]] | ⚪ draft | No iniciado — sin cola de trabajos ni worker threads |
| 5 | [[Sprint 5 - Whisper Integration]] | ✅ done | Implementado y verificado end-to-end 2026-07-28 (audio real → whisper.cpp → JSON + TXT, `interviews.status = completed`). `transcriptionController.h` sigue sin usarse (la transcripción corre dentro de `InterviewProcessingJobHandler`, no por un endpoint propio) |
| 6 | [[Sprint 6 - Ollama Integration]] | ⚪ draft | No iniciado |
| 7 | [[Sprint 7 - Export]] | ⚪ draft | No iniciado |
| 8 | [[Sprint 8 - Frontend]] | 🟠 in-progress | Adelantado fuera de orden; falta progreso real, descargas y configuración |
| 9 | [[Sprint 9 - Configuration]] | ⚪ draft | No iniciado |
| 10 | [[Sprint 10 - Testing]] | ⚪ draft | No iniciado — sin tests Catch2, solo un test manual de conexión |
| 11 | [[Sprint 11 - Documentation]] | 🟡 draft | Subestimado: `.ai/`, esta bóveda y `docs/API_REQUIREMENTS.md` ya existen; falta guía de instalación formal |
| 12 | [[Sprint 12 - Release]] | ⚪ draft | No iniciado |

Bloqueador clave: [[Sprint 4 - Background Processing]] abre el camino a 5, 6 y 7 (el pipeline de IA local, el corazón del producto).

# Why it matters

El roadmap traduce los principios de Hermes (ver [[Hermes - Vision General]]) en entregables concretos, incrementales, donde cada sprint debe dejar el proyecto en estado desplegable (README.md: "Cada Sprint debe producir una aplicación funcionando").

# Best Practices

- No avanzar a un sprint sin cerrar los entregables del anterior que sean bloqueantes.
- Cada sprint debe mantener el proyecto en un estado desplegable, sin romper funcionalidad previa.

# Common Mistakes

- Adelantar trabajo de un sprint posterior (por ejemplo, exportación a PDF) antes de tener la base de persistencia y transcripción estable.

# Hermes Usage

Esta nota es el índice de la sección [[04 Roadmap]] y debe mantenerse sincronizada con el estado real del proyecto.

# Related Notes

- [[Hermes - Vision General]]

# References

- README.md (Priority 2 — usado por ausencia de contenido en .ai/ROADMAP.md, Priority 1)
