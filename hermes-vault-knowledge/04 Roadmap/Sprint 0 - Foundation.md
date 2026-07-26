---
title: Sprint 0 - Foundation
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Roadmap General de Hermes", "Sprint 1 - Core API", "Compilar y Ejecutar el Backend con CMake"]
---

# Summary

Preparar completamente el entorno de desarrollo de Hermes.

# Explanation

**Objetivo:** dejar listo el entorno base del proyecto.

**Entregables:**
- Repositorio Git
- Configuración CMake
- Configuración vcpkg
- Primer proyecto compilando
- Configuración de Crow
- Configuración SQLite
- Configuración Logging
- Primer endpoint REST
- Documentación inicial

**Endpoints:** `GET /api/v1/health`, `GET /api/v1/info`

# Why it matters

Sin un entorno reproducible (CMake + vcpkg) y un endpoint mínimo funcionando, ningún sprint posterior puede validarse de forma consistente entre máquinas.

# Best Practices

- Fijar versiones de dependencias en vcpkg desde el primer commit.

# Common Mistakes

- Empezar a escribir lógica de dominio antes de tener el build y el logging funcionando.

# Hermes Usage

Primer sprint de [[Roadmap General de Hermes]]; base para [[Sprint 1 - Core API]].

# Related Notes

- [[Roadmap General de Hermes]]
- [[Sprint 1 - Core API]]
- [[Compilar y Ejecutar el Backend con CMake]]

# References

- README.md (Priority 2)
