---
title: Politica de Fuentes Tecnicas
aliases: ["docs/documentation_tech.yml", "Fuentes Oficiales Aprobadas"]
tags: [reference, policy, sources]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: docs/documentation_tech.yml
related: ["MOC - Referencias", "Alcance y Exclusiones"]
---

# Summary

Política formal de priorización de fuentes técnicas de Hermes, definida en `docs/documentation_tech.yml` (v1.0): qué priorizar y qué evitar al documentar o investigar.

# Explanation

`docs/documentation_tech.yml` es un archivo estructurado (no listado dentro de `.ai/`, pero autodescrito como "Official documentation sources approved for the Hermes project") que formaliza dos reglas:

**Orden de prioridad** (`rules.priority`):
1. Official Documentation
2. Official GitHub Repository
3. Language Reference
4. Microsoft Learn
5. Standards (ISO / WG21)
6. Approved Books

**Fuentes a evitar** (`rules.avoid`):
- Blogs aleatorios
- Medium
- Stack Overflow como documentación primaria
- Tutoriales generados por IA sin verificación

Además, define 32 categorías de fuentes oficiales (`sources.*`), cubriendo desde el lenguaje C++ hasta Obsidian y MCP — usadas para poblar y actualizar toda la sección [[14 References]] el 2026-07-24.

# Why it matters

Esta política es más granular que la jerarquía general del curador (Priority 1 a 5) y más completa que `.ai/OFFICIAL_SOURCES.md` (que solo listaba 9 URLs). Formaliza explícitamente lo que antes era implícito (evitar Medium/blogs/Stack Overflow como fuente primaria).

# Best Practices

Consultar `docs/documentation_tech.yml` como catálogo de fuentes antes de investigar un tema nuevo del Knowledge Backlog, en vez de buscar libremente en la web.

# Common Mistakes

Tratar un resultado de Stack Overflow o un tutorial generado por IA como si fuera equivalente a la documentación oficial — está explícitamente en la lista `avoid` de este archivo.

# Hermes Usage

Fuente usada para la actualización masiva de [[14 References]] realizada el 2026-07-24 (fuentes de C++, build tooling, testing, frontend, IA local, y tooling general).

**Nota de coherencia:** existen ahora dos listados de fuentes oficiales en el repositorio — `.ai/OFFICIAL_SOURCES.md` (Priority 1, 9 URLs) y `docs/documentation_tech.yml` (más completo, 32 categorías). Se sugiere al equipo del proyecto reconciliar ambos archivos (por ejemplo, consolidando `.ai/OFFICIAL_SOURCES.md` para que apunte a `docs/documentation_tech.yml`) para evitar que diverjan con el tiempo. Esta bóveda no modifica `.ai/` directamente, solo lo señala como sugerencia.

# Related Notes

- [[MOC - Referencias]]
- [[Alcance y Exclusiones]]

# References

- docs/documentation_tech.yml (v1.0)
- .ai/OFFICIAL_SOURCES.md (Priority 1 — listado más corto y previo)
