---
title: HTTP - Referencia MDN
aliases: ["HTTP"]
tags: [reference, http, web]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://developer.mozilla.org/docs/Web/HTTP
related: ["API First", "ADR-003 - Crow como Framework REST", "REST API Design - Microsoft Learn"]
---

# Summary

Referencia oficial de MDN sobre el protocolo HTTP: métodos, códigos de estado y cabeceras.

# Explanation

Fuente: https://developer.mozilla.org/docs/Web/HTTP

Verificado 2026-07-24, cubre tres áreas principales:
- **Métodos** (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, `OPTIONS`, `CONNECT`, `TRACE`).
- **Códigos de estado**, agrupados en 5 clases (informativos, éxito, redirección, error de cliente, error de servidor), desde `100` hasta `511`.
- **Cabeceras**, con más de 160 documentadas (`Content-Type`, `Authorization`, `Cache-Control`, cabeceras CORS y de seguridad como `Content-Security-Policy`).

# Why it matters

Es la base semántica de toda la REST API de Hermes construida con [[ADR-003 - Crow como Framework REST|Crow]] (ver [[API First]]): los endpoints de Interview/Configuration/Health deben usar los métodos y códigos de estado correctos (por ejemplo, `201` al crear una entrevista, `404` si no existe).

# Best Practices

Usar códigos de estado semánticamente correctos (no devolver siempre `200` con un campo `"error"` en el body).

# Common Mistakes

Usar `GET` para operaciones que modifican estado (por ejemplo, iniciar una transcripción), violando la semántica HTTP y rompiendo el cacheo/idempotencia esperada.

# Hermes Usage

Referencia base para diseñar los endpoints de todos los sprints del [[04 Roadmap]] que exponen REST API.

# Related Notes

- [[API First]]
- [[ADR-003 - Crow como Framework REST]]
- [[REST API Design - Microsoft Learn]]

# References

- https://developer.mozilla.org/docs/Web/HTTP (Priority 2, docs/documentation_tech.yml)
