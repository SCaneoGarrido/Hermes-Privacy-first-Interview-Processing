---
title: ADR-011 - Contrato de Respuesta API Uniforme
aliases: ["API Response Contract"]
tags: [adr, hermes, architecture]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: backend/api/rules/contract.md
related: ["API First", "ADR-003 - Crow como Framework REST", "Sprint 1 - Core API"]
---

# Context

Con varios endpoints REST ya implementados (`/health`, `/upload`, `/interview`), hacía falta fijar una única forma de respuesta para que el frontend ([[Sprint 8 - Frontend]]) pudiera parsear cualquier respuesta sin lógica especial por endpoint. El riesgo, sin esta regla, es que cada controller invente su propia forma de reportar errores.

# Decision

**Todas** las respuestas HTTP de la API, sin excepción, siguen el mismo envelope JSON:

```json
{
  "success": true | false,
  "data": <objeto | array | null>,
  "error": { "code": "SCREAMING_SNAKE_CASE", "message": "..." } | null
}
```

Si `success: true` → `error` es `null`. Si `success: false` → `data` es `null`. Esto incluye las respuestas que Crow genera por defecto y que normalmente escaparían al control de un controller: 404 (ruta inexistente), 405 (método no permitido) y excepciones no capturadas.

# Alternatives

No se documentan alternativas explícitas; la regla nace directamente de `backend/api/rules/contract.md` ("Nunca romper este contrato. Nunca devolver una estructura diferente por conveniencia").

# Consequences

- `ApiResponse::success(...)` / `ApiResponse::failure(...)` (`backend/api/include/ApiResponse.h`) son el único punto donde se construye una respuesta; ningún controller arma JSON de respuesta a mano.
- Sin intervención extra, Crow responde 404/405 con body vacío o texto plano, rompiendo el contrato. Se resuelve registrando un `CROW_CATCHALL_ROUTE(app)` en `main.cpp` que intercepta ambos casos y responde con `NOT_FOUND` / `METHOD_NOT_ALLOWED` en el mismo formato.
- Sin intervención extra, una excepción no capturada por el `try/catch` de un controller cae en el `default_exception_handler` de Crow (500 con body vacío, o 400 con texto plano para `crow::bad_request`). Se resuelve con `app.exception_handler(...)` en `main.cpp`, que re-empaqueta cualquier excepción como `ApiResponse::failure(...)`.
- El frontend puede tener un único cliente HTTP genérico (`frontend/src/api/client.ts`) que interpreta `success`/`error` de forma uniforme, sin casos especiales por endpoint — incluye endpoints que todavía no existen en el backend: como el catchall ya devuelve `NOT_FOUND` en el contrato correcto, la UI los muestra como "no disponible" en vez de romperse.

# Status

Accepted

# References

- backend/api/rules/contract.md (Priority 1)
- backend/api/include/ApiResponse.h
- backend/main.cpp (`CROW_CATCHALL_ROUTE`, `app.exception_handler`)
