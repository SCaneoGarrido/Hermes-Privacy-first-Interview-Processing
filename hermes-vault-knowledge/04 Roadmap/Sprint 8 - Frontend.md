---
title: Sprint 8 - Frontend
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 7 - Export", "Sprint 9 - Configuration", "API First", "ADR-011 - Contrato de Respuesta API Uniforme", "Sprint 4 - Background Processing"]
---

# Summary

Interfaz web en React para operar todo el pipeline de Hermes.

# Explanation

**Objetivo:** interfaz web.

**Entregables (2026-07-28)**, adelantados fuera de orden respecto al roadmap original (los sprints 4-7 todavía no existen) porque el objetivo inmediato era validar el flujo crear → subir audio → procesar de punta a punta:
- ✅ React + TypeScript + Vite, sin librería de UI ni autenticación (fuera de alcance v1.0)
- ✅ Carga de entrevistas (crear + asociar audio)
- ✅ Listado (`InterviewsListPage`, consume `GET /interviews`)
- ✅ Detalle (`InterviewDetailPage`, consume `GET /interview/:id`, incluye subir audio y botón "Enviar a procesar")
- ❌ Progreso — no hay cola de trabajos real todavía ([[Sprint 4 - Background Processing]] no implementado); el botón "procesar" solo marca `status = processing`, sin pipeline detrás
- ❌ Descargas — depende de [[Sprint 7 - Export]], no implementado
- ❌ Configuración — depende de [[Sprint 9 - Configuration]], no implementado

# Why it matters

Es el único punto de interacción humana con el sistema; debe consumir la REST API existente sin introducir lógica de negocio propia ([[API First]]).

# Best Practices

- El frontend solo orquesta llamadas a la API y presenta estado (`pending_audio` / `pending_processing` / `processing` / `completed` / `failed`, ver [[Sprint 4 - Background Processing]]).
- Usar un proxy de dev server (`vite.config.ts`) hacia el backend en vez de configurar CORS, mientras backend y frontend se sirvan desde el mismo origen en producción.
- Diseñar el cliente HTTP para tolerar endpoints que todavía no existen en el backend (ver [[ADR-011 - Contrato de Respuesta API Uniforme]]): mostrar "no disponible" en vez de romper la UI.

# Common Mistakes

- Implementar validaciones de negocio (por ejemplo, reglas de anonimización) en el cliente React en vez de en el backend.

# Hermes Usage

Consume los endpoints documentados en `docs/API_REQUIREMENTS.md` — el contrato completo (qué existe, qué falta, y por qué) vive ahí en vez de en el código del frontend únicamente. El detalle técnico completo de la API que el frontend consume está en [[Sprint 2 - Persistence]] y [[Sprint 3 - File Upload]].

# Related Notes

- [[Sprint 7 - Export]]
- [[Sprint 9 - Configuration]]
- [[API First]]
- [[ADR-011 - Contrato de Respuesta API Uniforme]]
- [[Sprint 4 - Background Processing]]

# References

- README.md (Priority 2)
- docs/API_REQUIREMENTS.md (Priority 1)
