---
title: ADR-012 - Repository y Service Layer para Entrevistas
aliases: ["IInterviewRepository", "InterviewService"]
tags: [adr, hermes, architecture]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: .ai/DECISIONS.md
related: ["Filosofia de Repositorios", "Sprint 2 - Persistence", "Sprint 4 - Background Processing", "Arquitectura en Capas de Hermes", "ADR-008 - MySQL como Base de Datos"]
---

# Context

Desde el Sprint 2, `InterviewController` y `FileController` llamaban directamente a `DatabaseManager::getInstance()` con SQL inline, una desviación explícita y aceptada de [[Filosofia de Repositorios]] para poder validar rápido el flujo end-to-end (crear → subir audio → procesar). Antes de arrancar [[Sprint 4 - Background Processing]] (cola de trabajos, worker threads), que iba a sumar más call sites acoplados a `DatabaseManager`, se decidió cerrar la desviación mientras el costo de hacerlo todavía era bajo (solo 2 controllers involucrados).

# Decision

Introducir `IInterviewRepository` (interfaz pura) implementada por `MySqlInterviewRepository`, y una capa `InterviewService` fina entre los controllers y el repositorio:

```
Controller → InterviewService → IInterviewRepository → MySqlInterviewRepository → DatabaseManager
```

`InterviewService` no depende de Crow ni de MySQL — expone resultados vía structs/enums propios (`InterviewRecord`, `ProcessOutcome`, `RemoveOutcome`) para que los controllers traduzcan a HTTP sin que el servicio conozca el protocolo.

# Alternatives

- Saltar directo de `Controller` a `IInterviewRepository` sin `InterviewService`: descartado porque las reglas de negocio existentes (ej. "requiere audio antes de procesar", "borrar el archivo físico al eliminar una entrevista") habrían quedado en el controller o en el repositorio, ninguno de los dos lugares correctos según [[Arquitectura en Capas de Hermes]].
- Posponer el refactor hasta después de [[Sprint 4 - Background Processing]]: descartado porque el costo de retrofitear la abstracción crece con cada nuevo call site a `DatabaseManager` que agregue la cola de trabajos.

# Consequences

- `InterviewController` y `FileController` ya no importan `DatabaseManager.h`; ambos reciben `InterviewService&` por constructor, cableado en el composition root (`main.cpp`).
- Se aprovechó el mismo cambio para agregar `DELETE /interview/:id` (cierra [[Sprint 2 - Persistence]]): `InterviewService::removeInterview` borra la fila (la cascada de MySQL se encarga de `interviews_audio`/`interview_results`) y además borra el archivo de audio real en disco — necesario por Privacy First, sin eso "eliminar" una entrevista dejaría el audio huérfano en `./uploads`.
- El versionado de API (`/api/v1`) quedó explícitamente fuera de este esfuerzo — decisión tomada aparte, no resuelta por esta ADR.

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- docs/API_REQUIREMENTS.md (Priority 1 — contrato de `DELETE /interview/:id`)
