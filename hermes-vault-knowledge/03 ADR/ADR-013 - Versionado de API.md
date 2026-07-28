---
title: ADR-013 - Versionado de API
aliases: ["API Versioning", "/api/v1"]
tags: [adr, hermes, architecture]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: .ai/DECISIONS.md
related: ["Sprint 1 - Core API", "Sprint 0 - Foundation", "API First", "ADR-011 - Contrato de Respuesta API Uniforme", "ADR-012 - Repository y Service Layer para Entrevistas"]
---

# Context

[[Sprint 1 - Core API]] proponía versionar la API (`/api/v1/...`) desde ese sprint, pero se pospuso deliberadamente para no frenar el flujo end-to-end (crear → subir audio → procesar). El resultado fue que todos los endpoints quedaron como rutas planas (`/health`, `/interview`, `/interviews`, etc. — ver [[Sprint 0 - Foundation]]).

# Decision

Prefijar todas las rutas del backend con `/api/v1` (`backend/main.cpp`). El cliente HTTP del frontend (`frontend/src/api/client.ts`) pasa a construir requests contra `BASE_URL = "/api/v1"`, y el proxy de Vite (`frontend/vite.config.ts`) reenvía `/api/*` al backend **sin reescribir el path**, porque el backend ya sirve exactamente ese prefijo.

# Alternatives

- Seguir sin versionar: descartado — quedaba como deuda abierta desde Sprint 1 y cada endpoint nuevo la hacía más costosa de resolver después.

# Consequences

- **Riesgo real detectado en la práctica**: el prefijo se agregó primero solo en el backend (`main.cpp`), sin tocar el frontend ni el proxy de Vite, lo que rompió toda la app (404 en cada request) hasta que se corrigieron `client.ts` y `vite.config.ts` en el mismo esfuerzo. Cualquier cambio de prefijo de rutas en el backend debe ir acompañado del cliente HTTP y del proxy de dev en el mismo cambio.
- `docs/API_REQUIREMENTS.md` documenta cada endpoint ya con el path real (`/api/v1/...`).

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- docs/API_REQUIREMENTS.md (Priority 1)
