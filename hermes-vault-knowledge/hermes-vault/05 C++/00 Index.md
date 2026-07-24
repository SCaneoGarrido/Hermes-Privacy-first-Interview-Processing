---
title: 05 C++ - Index
aliases: ["Cpp Index", "05 C++"]
tags: [moc, cpp, index]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["ADR-002 - C++20 como Lenguaje", "Hermes Coding Standard", "Libro - Professional C++ 5th Edition (Marc Gregoire)"]
---

# Summary

Índice de notas atómicas de C++ moderno relevantes para Hermes. Sección pendiente de desarrollo.

# Explanation

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md` aún sin nota atómica dedicada:

**Alta prioridad**
- [ ] RAII (ver definición corta en [[RAII]])
- [ ] Smart Pointers (`unique_ptr`, `shared_ptr`, `weak_ptr`)

**Baja prioridad**
- [ ] Coroutines
- [ ] SIMD
- [ ] Benchmarking

Fuente prevista: [[Libro - Professional C++ 5th Edition (Marc Gregoire)]] y cppreference (Priority 5, cuando haya acceso a Internet).

# Why it matters

Estos conceptos fundamentan directamente la [[Hermes Coding Standard]] (uso obligatorio de RAII, prohibición de `new`/`delete` crudos, preferencia por `unique_ptr`).

# Best Practices

Al crear cada nota, seguir la [[Plantilla - Nota Atomica]] y enlazarla desde este índice.

# Common Mistakes

No crear una nota monolítica "C++ Moderno"; cada concepto (RAII, cada tipo de smart pointer) debe ser una nota separada.

# Hermes Usage

Ver uso concreto de estos conceptos en [[Hermes Coding Standard]] y [[ADR-002 - C++20 como Lenguaje]].

# Related Notes

- [[ADR-002 - C++20 como Lenguaje]]
- [[Hermes Coding Standard]]
- [[Libro - Professional C++ 5th Edition (Marc Gregoire)]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
