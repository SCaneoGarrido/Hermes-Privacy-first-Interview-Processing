---
title: Sprint 0 - Foundation
aliases: []
tags: [roadmap, sprint, hermes]
status: done
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Roadmap General de Hermes", "Sprint 1 - Core API", "Compilar y Ejecutar el Backend con CMake", "ADR-010 - Docker Compose para MySQL Local"]
---

# Summary

Preparar completamente el entorno de desarrollo de Hermes.

# Explanation

**Objetivo:** dejar listo el entorno base del proyecto.

**Entregables (2026-07-28 — completos):**
- Repositorio Git
- Configuración CMake (`backend/CMakeLists.txt`, triplet `x64-mingw-static`)
- Configuración vcpkg (`backend/vcpkg.json`)
- Primer proyecto compilando (`backend.exe`)
- Configuración de Crow
- Configuración MySQL (Docker Compose + libmariadb — ver [[ADR-010 - Docker Compose para MySQL Local]], reemplaza el plan original de SQLite)
- Configuración Logging (`logger.cpp`, `log_event`, escribe a `API.log`)
- Primer endpoint REST (`GET /health`)
- Documentación inicial (`docs/`, `.ai/`, esta bóveda)

**Endpoints:** implementado como `GET /health` (no `GET /api/v1/health` como decía el plan original — el prefijo `/api/v1` nunca se adoptó; los endpoints reales son rutas planas: `/health`, `/upload`, `/file/<id>`, `/interview`, `/interviews`, `/interview/<id>`, `/interview/<id>/process`). `GET /api/v1/info` nunca se implementó.

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
