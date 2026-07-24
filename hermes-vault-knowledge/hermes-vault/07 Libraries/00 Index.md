---
title: 07 Libraries - Index
aliases: ["Libraries Index", "07 Libraries"]
tags: [moc, libraries, index]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["SQLite - Documentacion Oficial", "React - Documentacion Oficial", "Stack Tecnologico de Hermes"]
---

# Summary

Índice de notas atómicas sobre librerías concretas usadas por Hermes. Sección pendiente de desarrollo.

# Explanation

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md`:

**Alta prioridad**
- [ ] SQLite Transactions
- [ ] React Query

**No listadas en el backlog pero presentes en el stack** (.ai/PROJECT.md), pendientes de evaluar prioridad:
- [ ] nlohmann/json (serialización)
- [ ] spdlog (logging)
- [ ] Catch2 (testing)
- [ ] vcpkg (gestión de paquetes)

# Why it matters

Estas librerías son las piezas concretas de infraestructura que deben quedar aisladas detrás de interfaces según la [[Filosofia de Repositorios]].

# Best Practices

Cada nota debe explicar específicamente cómo se usa la librería dentro de Hermes, no solo su API genérica.

# Common Mistakes

Documentar toda la superficie de la librería en una sola nota en vez de dividir por concepto (por ejemplo, "SQLite Transactions" separado de "SQLite Prepared Statements").

# Hermes Usage

Ver [[Stack Tecnologico de Hermes]] para la tabla completa de tecnologías.

# Related Notes

- [[SQLite - Documentacion Oficial]]
- [[React - Documentacion Oficial]]
- [[Stack Tecnologico de Hermes]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
- .ai/PROJECT.md (Priority 1)
