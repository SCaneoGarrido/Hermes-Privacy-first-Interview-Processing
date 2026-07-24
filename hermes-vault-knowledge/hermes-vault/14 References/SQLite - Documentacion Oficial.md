---
title: SQLite - Documentacion Oficial
aliases: []
tags: [reference, sqlite, libraries]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://sqlite.org/docs.html
related: ["ADR-006 - SQLite como Base de Datos"]
---

# Summary

Documentación oficial de SQLite, incluyendo la referencia del API en C (`c3ref`).

# Explanation

Fuentes oficiales:
- https://sqlite.org/docs.html
- https://sqlite.org/c3ref/intro.html (referencia del API en C, usada directamente desde C++ en Hermes)

# Why it matters

Es la base de datos embebida de Hermes ([[ADR-006 - SQLite como Base de Datos]]); su API en C es la que consumirán los repositorios de la capa `Infrastructure`.

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
