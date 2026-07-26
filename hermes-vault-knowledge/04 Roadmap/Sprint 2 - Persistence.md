---
title: Sprint 2 - Persistence
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 1 - Core API", "Sprint 3 - File Upload", "ADR-006 - SQLite como Base de Datos"]
---

# Summary

Persistencia de entrevistas mediante SQLite: CRUD, gestión de archivos y metadata.

# Explanation

**Objetivo:** persistencia de entrevistas.

**Entregables:** SQLite, CRUD de entrevistas, gestión de archivos, metadata, directorios de almacenamiento.

**Funcionalidades:** crear entrevista, eliminar entrevista, consultar entrevista, listado.

# Why it matters

Es el primer sprint que ejercita [[ADR-006 - SQLite como Base de Datos]] en la práctica y valida que el acceso a datos quede correctamente oculto detrás de repositorios.

# Best Practices

- Definir el repositorio de entrevistas como interfaz antes de implementarlo sobre SQLite.

# Common Mistakes

- Acoplar la estructura de la tabla SQLite directamente a los DTOs expuestos por la API.

# Hermes Usage

Valida en código la [[Filosofia de Repositorios]] definida a nivel de proyecto.

# Related Notes

- [[Sprint 1 - Core API]]
- [[Sprint 3 - File Upload]]
- [[ADR-006 - SQLite como Base de Datos]]

# References

- README.md (Priority 2)
