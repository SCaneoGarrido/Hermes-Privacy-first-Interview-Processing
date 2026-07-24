---
title: Sprint 8 - Frontend
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 7 - Export", "Sprint 9 - Configuration", "API First"]
---

# Summary

Interfaz web en React para operar todo el pipeline de Hermes.

# Explanation

**Objetivo:** interfaz web.

**Entregables:** React, carga de entrevistas, listado, detalle, progreso, descargas, configuración.

# Why it matters

Es el único punto de interacción humana con el sistema; debe consumir la REST API existente sin introducir lógica de negocio propia ([[API First]]).

# Best Practices

- El frontend solo orquesta llamadas a la API y presenta estado (Pending/Running/Completed/Failed de [[Sprint 4 - Background Processing]]).

# Common Mistakes

- Implementar validaciones de negocio (por ejemplo, reglas de anonimización) en el cliente React en vez de en el backend.

# Hermes Usage

Consume los endpoints construidos en los sprints 1 a 7.

# Related Notes

- [[Sprint 7 - Export]]
- [[Sprint 9 - Configuration]]
- [[API First]]

# References

- README.md (Priority 2)
