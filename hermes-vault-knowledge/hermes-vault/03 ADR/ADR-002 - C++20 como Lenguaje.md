---
title: ADR-002 - C++20 como Lenguaje
aliases: []
tags: [adr, hermes, cpp]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Stack Tecnologico de Hermes", "Hermes Coding Standard"]
---

# Context

Hermes necesita un lenguaje para el backend, con objetivos tanto de rendimiento como de aprendizaje por parte del autor del proyecto.

# Decision

Usar **C++20** como lenguaje principal del backend.

# Alternatives

No se documentan alternativas explícitas en .ai/DECISIONS.md; la elección está motivada directamente por los objetivos del proyecto.

# Consequences

- Se requiere seguir la [[Hermes Coding Standard]] (RAII, smart pointers, sin `new`/`delete` crudos).
- Habilita features modernas de C++20 (concepts, ranges, coroutines) evaluables en el backlog de conocimiento.
- Mayor curva de aprendizaje que lenguajes de más alto nivel, asumida como objetivo secundario del proyecto (README.md: "Aprender y aplicar C++ moderno").

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- README.md (Priority 2, objetivos secundarios)
