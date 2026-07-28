---
title: SQLite - Documentacion Oficial
aliases: []
tags: [reference, sqlite, libraries]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: https://sqlite.org/docs.html
related: ["ADR-006 - SQLite como Base de Datos", "MySQL - Documentacion Oficial"]
---

> [!note] No adoptada
> SQLite fue reemplazada por MySQL como base de datos de Hermes ([[ADR-008 - MySQL como Base de Datos|ADR-008]], 2026-07-28). Esta nota se conserva como referencia histórica/comparativa; ver [[MySQL - Documentacion Oficial]] para la fuente actual.

# Summary

Documentación oficial de SQLite, incluyendo la referencia del API en C (`c3ref`).

# Explanation

Fuentes oficiales:
- https://sqlite.org/docs.html
- https://sqlite.org/c3ref/intro.html (referencia del API en C, usada directamente desde C++ en Hermes)
- https://sqlite.org/lang.html (referencia del lenguaje SQL soportado; verificado 2026-07-24: documenta `BEGIN TRANSACTION`, `COMMIT TRANSACTION`, `ROLLBACK TRANSACTION` y savepoints — fuente directa para la nota pendiente "SQLite Transactions")

# Why it matters

Fue la base de datos embebida elegida originalmente para Hermes ([[ADR-006 - SQLite como Base de Datos]], superada por [[ADR-008 - MySQL como Base de Datos|ADR-008]]).

# Best Practices

Usar la referencia `c3ref` para el manejo correcto de transacciones y statements preparados, evitando SQL injection al construir queries.

# Common Mistakes

N/A — nota de referencia.

# Hermes Usage

Ver pendiente: nota atómica "SQLite Transactions" en [[07 Libraries]] (Knowledge Backlog, alta prioridad).

# Related Notes

- [[ADR-006 - SQLite como Base de Datos]]

# References

- https://sqlite.org/docs.html (Priority 2)
- https://sqlite.org/c3ref/intro.html (Priority 2)
- https://sqlite.org/lang.html (Priority 2, docs/documentation_tech.yml)
