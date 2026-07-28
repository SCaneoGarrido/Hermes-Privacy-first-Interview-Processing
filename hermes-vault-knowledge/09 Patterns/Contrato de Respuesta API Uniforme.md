---
title: Contrato de Respuesta API Uniforme
aliases: ["API Response Contract", "Uniform Response Envelope"]
tags: [patterns, hermes, api]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: backend/api/rules/contract.md
related: ["ADR-011 - Contrato de Respuesta API Uniforme", "API First", "Sprint 8 - Frontend"]
---

# Summary

Patrón: toda respuesta HTTP de la API usa el mismo envelope JSON (`success`/`data`/`error`), sin excepciones — incluyendo las que Crow genera por defecto (404, 405, excepciones no capturadas).

# Explanation

```json
{
  "success": true | false,
  "data": <objeto | array | null>,
  "error": { "code": "SCREAMING_SNAKE_CASE", "message": "..." } | null
}
```

Ver [[ADR-011 - Contrato de Respuesta API Uniforme]] para la decisión completa. Este nota documenta el patrón en sí, reutilizable más allá de Hermes.

Tres piezas lo hacen cumplirse siempre, no solo en el "camino feliz":

1. **`ApiResponse::success`/`ApiResponse::failure`** (`backend/api/include/ApiResponse.h`) — único punto donde se construye el JSON de respuesta.
2. **`CROW_CATCHALL_ROUTE(app)`** en `main.cpp` — sin esto, una ruta inexistente (404) o un método no soportado (405) devuelven la respuesta por defecto de Crow (body vacío o texto plano), rompiendo el contrato para cualquier cliente que asuma siempre JSON.
3. **`app.exception_handler(...)`** en `main.cpp` — red de seguridad para cualquier excepción que se escape de los `try/catch` de cada controller; sin esto cae en el manejador por defecto de Crow (500 con body vacío, o 400 con texto plano para `crow::bad_request`).

# Why it matters

Sin este patrón, un cliente (el frontend, o cualquier integración futura) tiene que manejar N formas distintas de error según el endpoint y según si el fallo vino de un controller, un middleware, o del propio framework. Con el patrón, hay un único punto de parseo (`frontend/src/api/client.ts::request`).

# Best Practices

- Verificar el contrato con casos reales, no solo el camino feliz: probar explícitamente una ruta inexistente, un método no soportado, y forzar una excepción — no asumir que "anda en el caso normal" alcanza.
- Diseñar el cliente para que un endpoint faltante (`404 NOT_FOUND`) sea un estado manejable de la UI ("no disponible todavía"), no un crash — esto permite construir el frontend contra un backend incompleto sin bloquear el trabajo.

# Common Mistakes

- Registrar `CROW_CATCHALL_ROUTE` o `app.exception_handler` pero olvidarse de que también hay que registrarlos *antes* de considerar el contrato "cerrado" — es fácil dar por terminado el trabajo después de cubrir los controllers y olvidar estos dos casos límite, que solo se notan al probar una ruta que no existe.
- Devolver `""` (string vacío) como `message` de error "porque total ya se ve el `code`" — inútil para quien debug ea desde el otro lado del contrato.

# Hermes Usage

Todos los endpoints de Hermes (`/health`, `/upload`, `/interview`, `/interviews`, `/interview/:id`, `/interview/:id/process`) pasan por este contrato. El frontend (`frontend/src/api/client.ts`) lo asume ciegamente: cualquier `success:false` se convierte en una excepción `ApiError` tipada, con `code` y `message`.

# Related Notes

- [[ADR-011 - Contrato de Respuesta API Uniforme]]
- [[API First]]
- [[Sprint 8 - Frontend]]

# References

- backend/api/rules/contract.md (Priority 1)
