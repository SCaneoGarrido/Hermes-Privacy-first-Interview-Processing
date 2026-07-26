---
title: DTO
aliases: ["Data Transfer Object"]
tags: [glossary, pattern]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["09 Patterns", "Arquitectura en Capas de Hermes"]
---

# Summary

Objeto simple usado para transportar datos entre la capa REST API y la capa Application, sin exponer las entidades de dominio.

# Explanation

Nota de glosario (definición corta). El desarrollo completo del concepto —diseño concreto de los DTOs de Hermes (Interview, Configuration, etc.)— está pendiente en [[09 Patterns]] (Knowledge Backlog, prioridad media, "DTO Pattern").

# Why it matters

Evita que cambios en el modelo de dominio rompan el contrato público de la API, y evita filtrar detalles internos del dominio al cliente React.

# Best Practices

Ver nota completa pendiente en [[09 Patterns]].

# Common Mistakes

Serializar directamente una entidad de dominio como respuesta JSON de la API, saltándose el DTO.

# Hermes Usage

Mencionado como entregable explícito de [[Sprint 1 - Core API]].

# Related Notes

- [[09 Patterns]]
- [[Arquitectura en Capas de Hermes]]

# References

- .ai/PROJECT.md (Priority 1)
