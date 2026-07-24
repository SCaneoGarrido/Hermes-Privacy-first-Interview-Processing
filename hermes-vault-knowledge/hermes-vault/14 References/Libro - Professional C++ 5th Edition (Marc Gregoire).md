---
title: Libro - Professional C++ 5th Edition (Marc Gregoire)
aliases: ["Professional C++"]
tags: [reference, book, cpp]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: docs/Libros/Professional C++, 5th Edition by Marc Gregoire-Wrox-9781119695400.pdf
related: ["ADR-002 - C++20 como Lenguaje", "05 C++"]
---

# Summary

Libro aprobado (Priority 3) de referencia para C++ moderno, base de las futuras notas atómicas de la sección [[05 C++]].

# Explanation

*Professional C++, 5th Edition*, de Marc Gregoire (Wrox). Disponible localmente en `docs/Libros/`.

Cubre C++ moderno en profundidad: RAII, smart pointers, move semantics, plantillas, concurrencia. Es la fuente principal prevista para desarrollar los temas de alta prioridad del Knowledge Backlog: RAII, Smart Pointers, Thread Pools, Coroutines.

No se reproduce contenido textual del libro; las notas atómicas derivadas resumen y adaptan los conceptos al uso concreto en Hermes.

# Why it matters

Sustenta técnicamente [[ADR-002 - C++20 como Lenguaje]] y la [[Hermes Coding Standard]] (RAII, `unique_ptr`, prohibición de `new`/`delete` crudos).

# Best Practices

Al generar notas de C++ desde este libro, verificar que los ejemplos sean coherentes con C++20 y con `.ai/CODING_STANDARD.md`.

# Common Mistakes

Documentar features de versiones de C++ no adoptadas por el proyecto (ver [[ADR-002 - C++20 como Lenguaje]]) sin aclararlo explícitamente.

# Hermes Usage

Fuente principal pendiente de explotar para poblar [[05 C++]] (ver nota índice de esa sección).

# Related Notes

- [[ADR-002 - C++20 como Lenguaje]]
- [[05 C++]]

# References

- docs/Libros/Professional C++, 5th Edition by Marc Gregoire-Wrox-9781119695400.pdf (Priority 3)
