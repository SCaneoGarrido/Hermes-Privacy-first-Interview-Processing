---
title: REST API Design - Microsoft Learn
aliases: ["REST API Design"]
tags: [reference, rest, api-design]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://learn.microsoft.com/azure/architecture/best-practices/api-design
related: ["API First", "ADR-003 - Crow como Framework REST", "HTTP - Referencia MDN"]
---

# Summary

Guía de buenas prácticas de Microsoft Learn para el diseño de APIs REST, aplicable al diseño de endpoints de Hermes.

# Explanation

Fuente: https://learn.microsoft.com/azure/architecture/best-practices/api-design (verificado 2026-07-24: "Learn how to apply best practices for designing RESTful web APIs that support platform independence and loose coupling for service evolution").

Cubre, entre otros temas: modelado de la API en torno a recursos (no acciones), uso correcto de verbos HTTP, versionado de la API, manejo de colecciones grandes (paginación), y códigos de estado apropiados — complementa la referencia de [[HTTP - Referencia MDN]] con el nivel de diseño de recursos.

# Why it matters

El "versionado API" es un entregable explícito de [[Sprint 1 - Core API]]; esta guía es la fuente de Priority 2 para decidir cómo versionar (`/api/v1/...`, ya usado en [[Sprint 0 - Foundation]]) y cómo modelar los recursos Interview/Configuration/Health.

# Best Practices

Modelar los endpoints en torno a sustantivos/recursos (`/api/v1/interviews/{id}`) en vez de verbos (`/api/v1/getInterview`), coherente con [[API First]].

# Common Mistakes

Diseñar endpoints tipo RPC sobre HTTP (`POST /api/v1/doTranscription`) en vez de modelar el recurso y su transición de estado (por ejemplo, `POST /api/v1/interviews/{id}/transcription`).

# Hermes Usage

Guía de diseño para todos los endpoints REST de Hermes, especialmente relevante en [[Sprint 1 - Core API]] y [[Sprint 4 - Background Processing]] (modelado de recursos asíncronos con estado Pending/Running/Completed/Failed).

# Related Notes

- [[API First]]
- [[ADR-003 - Crow como Framework REST]]
- [[HTTP - Referencia MDN]]

# References

- https://learn.microsoft.com/azure/architecture/best-practices/api-design (Priority 2, docs/documentation_tech.yml)
