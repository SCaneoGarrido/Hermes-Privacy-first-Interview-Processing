---
title: Roadmap General de Hermes
aliases: ["Hermes Roadmap"]
tags: [roadmap, hermes]
status: stable
created: 2026-07-24
updated: 2026-10-02
source: .ai/ROADMAP.md
related: ["Hermes - Vision General"]
---

# Summary

Hermes se desarrolla en 13 sprints (Sprint 0 a Sprint 12), desde la configuración del entorno hasta la primera versión estable 1.0. **Se preparó una primera pre-release, v0.1.0, el 2026-08-15** (README, CHANGELOG y versión de CMake actualizados; commit pendiente al momento de escribir esta nota, todavía sin tag de git) — funcional de punta a punta pero sin cubrir el alcance completo de v1.0 (ver tabla de estado abajo).

# Explanation

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

# Status (2026-10-02)

| # | Sprint | Estado | Nota |
|---|--------|--------|------|
| 0 | [[Sprint 0 - Foundation]] | ✅ done | Entorno, CMake/vcpkg, Crow, MySQL, logging, `/health` |
| 1 | [[Sprint 1 - Core API]] | 🟠 in-progress | Services/Repositories y versionado `/api/v1` ya resueltos ([[ADR-012 - Repository y Service Layer para Entrevistas]], [[ADR-013 - Versionado de API]]); falta endpoint Configuration y DTOs formales |
| 2 | [[Sprint 2 - Persistence]] | ✅ done | CRUD completo (incl. `DELETE`) + `IInterviewRepository`/`InterviewService` ([[ADR-012 - Repository y Service Layer para Entrevistas]]). 2026-10-02: reconexión automática a MySQL en `DatabaseManager` |
| 3 | [[Sprint 3 - File Upload]] | 🟠 in-progress | Upload + firma de bytes, video .mp4, tope 1 GB. 2026-10-02: reemplazo de audio, validación previa (404/409) y cero archivos huérfanos. Falta progreso de subida |
| 4 | [[Sprint 4 - Background Processing]] | ✅ done | Cola de jobs in-memory + worker pool, máquina de estados persistida, dedupe (409), recuperación de crashes (`reclaimStuckJobs`), cierra una condición de carrera en `DatabaseManager` que existía desde Sprint 0 |
| 5 | [[Sprint 5 - Whisper Integration]] | ✅ done | Verificado end-to-end contra una entrevista real de ~70min (1804 segmentos) |
| 6 | [[Sprint 6 - Ollama Integration]] | 🟡 done (con reservas) | Verificado a escala real. Determinismo y normalización de etiquetas resueltos 2026-08-15. **Pendiente, bloquea Sprint 7**: recall de anonimización incompleto |
| 7 | [[Sprint 7 - Export]] | ⚪ draft (bloqueado) | No iniciado a propósito — depende de que la anonimización de Sprint 6 sea confiable. La vista de lectura con PDF desde el navegador (ADR-019) **no** es Export |
| 8 | [[Sprint 8 - Frontend]] | 🟢 casi completo | 2026-10-02: rediseño visual (identidad griega), lista con filtros, detalle por estado, vista de lectura con PDF. Falta configuración (depende de Sprint 9) |
| 9 | [[Sprint 9 - Configuration]] | ⚪ draft | No iniciado — toda la configuración hoy es por variables de entorno |
| 10 | [[Sprint 10 - Testing]] | ⚪ draft | No iniciado — sin tests Catch2, solo verificación manual contra entrevistas reales |
| 11 | [[Sprint 11 - Documentation]] | 🟡 in-progress (subestimado) | `.ai/`, esta bóveda y `docs/API_REQUIREMENTS.md` activamente mantenidos; `README.md` reescrito y `CHANGELOG.md` agregado 2026-08-15; falta guía de instalación formal |
| 12 | [[Sprint 12 - Release]] | 🟡 en curso | v0.1.0 (pre-release/early preview) preparada 2026-08-15 — ver [[Estado Actual del Proyecto]]. No es la v1.0 completa que este sprint define originalmente |

Bloqueador clave hoy: el recall de anonimización de [[Sprint 6 - Ollama Integration]] bloquea [[Sprint 7 - Export]]. Para el estado de trabajo en curso, sesión a sesión, ver [[Estado Actual del Proyecto]].

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
- [[Estado Actual del Proyecto]]

# References

- .ai/ROADMAP.md (Priority 1)
- README.md (Priority 2, complementario — reescrito 2026-08-15 para explicar el proyecto tal como es hoy)
