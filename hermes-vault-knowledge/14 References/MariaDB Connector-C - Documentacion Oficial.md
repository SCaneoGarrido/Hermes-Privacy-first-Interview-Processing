---
title: MariaDB Connector-C - Documentacion Oficial
aliases: ["libmariadb docs"]
tags: [reference, mariadb, libraries]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: https://mariadb.com/kb/en/mariadb-connector-c/
related: ["ADR-009 - libmariadb como Cliente MySQL", "MariaDB Connector-C (libmariadb)"]
---

# Summary

Documentación oficial de MariaDB Connector/C (`libmariadb`), el cliente C que Hermes usa para hablar con MySQL desde C++ ([[ADR-009 - libmariadb como Cliente MySQL]]).

# Explanation

Fuentes oficiales:
- https://mariadb.com/kb/en/mariadb-connector-c/ — documentación de la librería
- https://mariadb.com/docs/server/connect/programming-languages/c/
- https://github.com/mariadb-corporation/mariadb-connector-c — repositorio oficial (fuente del puerto de vcpkg)

# Why it matters

Es compatible con el protocolo clásico de MySQL (no solo MariaDB), y es el cliente elegido en vez del connector oficial de Oracle por compatibilidad con MinGW — ver [[ADR-009 - libmariadb como Cliente MySQL]] para el detalle completo.

# Best Practices

Ver [[MariaDB Connector-C (libmariadb)]] para las prácticas concretas de uso en Hermes (prepared statements, RAII, el gotcha de `mysql_library_init` en MinGW).

# Common Mistakes

Ver [[MariaDB Connector-C (libmariadb)]].

# Hermes Usage

Consumido a través de `DatabaseManager` (`backend/api/shared/database/`), nunca directamente desde los controllers en el diseño ideal (ver desviación conocida en [[Sprint 2 - Persistence]]).

# Related Notes

- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[MariaDB Connector-C (libmariadb)]]

# References

- https://mariadb.com/kb/en/mariadb-connector-c/ (Priority 2)
- https://github.com/mariadb-corporation/mariadb-connector-c (Priority 4)
