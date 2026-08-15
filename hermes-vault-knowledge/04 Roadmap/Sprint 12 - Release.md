---
title: Sprint 12 - Release
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-08-15
source: README.md
related: ["Sprint 11 - Documentation", "Roadmap General de Hermes", "Estado Actual del Proyecto"]
---

# Summary

Primera versión estable (1.0) de Hermes, con instalador para Windows y repositorio público.

# Explanation

**Objetivo:** primera versión estable.

**Entregables:** versión 1.0, release notes, instalador Windows, documentación, repositorio público.

> **2026-08-15 — Primera pre-release preparada, no la v1.0 de este sprint.** Dado el estado real del proyecto (sin Export, sin Configuration, sin tests, sin instalador, y con el recall de anonimización todavía incompleto), etiquetar el resultado como "1.0 stable" habría sido engañoso — la decisión, tomada junto con el usuario, fue lanzar **v0.1.0 (pre-release / early preview)** en su lugar: `CHANGELOG.md` nuevo con las notas de esta versión, `README.md` reescrito para explicar el proyecto tal como es hoy (con secciones explícitas "Qué falta para v1.0" y "Limitaciones conocidas"), y versión de CMake (`project(backend VERSION 0.1.0)`) alineada con `frontend/package.json` (ya estaba en `0.1.0`). Este sprint sigue abierto: falta el tag de git, el release en GitHub, release notes formales fuera del CHANGELOG, y por supuesto todo lo que la v1.0 real requiere (instalador Windows, documentación completa).

# Why it matters

Es el hito que materializa la misión del proyecto: un investigador puede instalar Hermes localmente sin conocimientos avanzados de programación.

# Best Practices

- Seguir Semantic Versioning (MAJOR.MINOR.PATCH) desde el primer release, según README.md.

# Common Mistakes

- Incluir en el release funcionalidades marcadas como fuera de alcance para v1.0 (ver [[Alcance y Exclusiones]]).

# Hermes Usage

Cierra el roadmap inicial descrito en [[Roadmap General de Hermes]].

# Related Notes

- [[Sprint 11 - Documentation]]
- [[Roadmap General de Hermes]]

# References

- README.md (Priority 2)
