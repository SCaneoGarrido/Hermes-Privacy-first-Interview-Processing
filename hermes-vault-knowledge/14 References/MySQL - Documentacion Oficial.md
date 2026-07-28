---
title: MySQL - Documentacion Oficial
aliases: []
tags: [reference, mysql, libraries]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: https://dev.mysql.com/doc/refman/en/
related: ["ADR-008 - MySQL como Base de Datos", "ADR-010 - Docker Compose para MySQL Local", "MariaDB Connector-C - Documentacion Oficial"]
---

# Summary

Documentación oficial del servidor MySQL, la base de datos de Hermes desde [[ADR-008 - MySQL como Base de Datos]].

# Explanation

Fuentes oficiales:
- https://dev.mysql.com/doc/refman/en/ — referencia completa del servidor y del lenguaje SQL
- https://hub.docker.com/_/mysql — imagen oficial usada en `docker-compose.yml` (ver [[ADR-010 - Docker Compose para MySQL Local]])

Hermes corre MySQL 8.0 (imagen `mysql:8.0`, verificado en runtime: 8.0.46).

# Why it matters

Es la base de datos de Hermes; su dialecto SQL y su comportamiento de bind de tipos (por ejemplo, columnas `DATETIME`) determinan cómo se escribe `DatabaseManager` (ver [[MariaDB Connector-C (libmariadb)]]).

# Best Practices

- MySQL, a diferencia de MariaDB, no soporta `ALTER TABLE ... ADD COLUMN IF NOT EXISTS` — las migraciones idempotentes en Hermes dependen de tolerar el error 1060 (`ER_DUP_FIELDNAME`) en código, no de esa sintaxis.
- Confirmar la versión real del servidor (`SELECT VERSION();`) antes de asumir soporte de una sintaxis específica.

# Common Mistakes

- Asumir compatibilidad 1:1 con sintaxis de MariaDB (por ejemplo, `IF NOT EXISTS` en `ADD COLUMN`) solo porque ambos motores comparten el protocolo de red y gran parte del dialecto SQL.

# Hermes Usage

`SQL/init.sql` es el script de schema (`CREATE TABLE IF NOT EXISTS` + `ALTER TABLE`), ejecutado por `DatabaseManager::migrateTables` en cada arranque del backend.

# Related Notes

- [[ADR-008 - MySQL como Base de Datos]]
- [[ADR-010 - Docker Compose para MySQL Local]]
- [[MariaDB Connector-C - Documentacion Oficial]]

# References

- https://dev.mysql.com/doc/refman/en/ (Priority 2)
- https://hub.docker.com/_/mysql (Priority 2)
