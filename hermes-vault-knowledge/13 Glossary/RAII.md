---
title: RAII
aliases: ["Resource Acquisition Is Initialization"]
tags: [glossary, cpp]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/CODING_STANDARD.md
related: ["Hermes Coding Standard", "05 C++"]
---

# Summary

Patrón de C++ donde la adquisición de un recurso se vincula a la construcción de un objeto y su liberación a la destrucción de ese objeto.

# Explanation

Nota de glosario (definición corta). El desarrollo completo del concepto —con ejemplos, casos de uso y buenas prácticas en C++20— está pendiente en [[05 C++]] (Knowledge Backlog, alta prioridad).

# Why it matters

Es uso obligatorio en Hermes según la [[Hermes Coding Standard]] ("Use RAII"), y es la base de por qué se prohíbe `new`/`delete` crudo.

# Best Practices

Ver nota completa pendiente en [[05 C++]].

# Common Mistakes

Ver nota completa pendiente en [[05 C++]].

# Hermes Usage

Todo manejo de recursos (memoria, archivos, conexiones) en Hermes debe seguir RAII, típicamente vía `unique_ptr`/`shared_ptr` o wrappers RAII propios sobre recursos de C (por ejemplo, handles de SQLite).

# Related Notes

- [[Hermes Coding Standard]]
- [[05 C++]]

# References

- .ai/CODING_STANDARD.md (Priority 1)
