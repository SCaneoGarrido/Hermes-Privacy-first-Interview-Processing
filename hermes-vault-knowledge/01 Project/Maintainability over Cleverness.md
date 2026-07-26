---
title: Maintainability over Cleverness
aliases: []
tags: [project, principle, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Hermes - Vision General", "Hermes Coding Standard"]
---

# Summary

Principio inmutable de Hermes: se prefiere código legible sobre optimizaciones complejas.

# Explanation

Ante una disyuntiva entre una solución sencilla y comprensible y una solución más "inteligente" pero difícil de mantener, Hermes elige la primera. Esto es coherente con la instrucción operativa de .ai/AI_INSTRUCTIONS.md: "Do not optimize prematurely", "Keep functions small", "Keep classes cohesive".

# Why it matters

Hermes es un proyecto open-source con objetivos de aprendizaje explícitos (C++ moderno) y con la meta de que un investigador o desarrollador nuevo pueda entender el sistema navegando su código y documentación, sin explicaciones adicionales.

# Best Practices

- Evitar plantillas (templates) o metaprogramación excesiva cuando una solución directa es suficiente.
- Documentar el porqué de una decisión no obvia en vez de dejar que el código "hable por sí solo" cuando no es evidente.

# Common Mistakes

- Introducir abstracciones genéricas para casos hipotéticos futuros que no existen todavía.
- Optimizar rutas de código sin evidencia de que sean un cuello de botella (ver Knowledge Backlog: Benchmarking).

# Hermes Usage

Este principio guía las revisiones de código y el diseño de la [[Hermes Coding Standard]].

# Related Notes

- [[Hermes - Vision General]]
- [[Hermes Coding Standard]]

# References

- .ai/PROJECT.md (Priority 1)
- .ai/AI_INSTRUCTIONS.md (Priority 1)
