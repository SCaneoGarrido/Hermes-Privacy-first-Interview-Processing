---
title: MariaDB Connector-C (libmariadb)
aliases: ["libmariadb"]
tags: [libraries, hermes, cpp]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: docs/decisions/0001-cliente-base-de-datos.md
related: ["ADR-009 - libmariadb como Cliente MySQL", "RAII", "MariaDB Connector-C - Documentacion Oficial"]
---

# Summary

Librería cliente en C usada por Hermes para hablar con MySQL desde C++ (ver [[ADR-009 - libmariadb como Cliente MySQL]]).

# Explanation

`libmariadb` se instala vía vcpkg (`"libmariadb"` en `backend/vcpkg.json`) y se enlaza con `find_package(unofficial-libmariadb CONFIG REQUIRED)` / `target_link_libraries(... unofficial::libmariadb)`. Es API en C (`mysql.h`), no C++ nativo — Hermes la envuelve en `DatabaseManager` (`backend/api/shared/database/`), que expone:

- `executePrepared(query, params, return_id=false)` — INSERT/UPDATE con `MYSQL_STMT`, devuelve `std::optional<uint64_t>` (el id insertado si `return_id=true`, o `0` en éxito sin id, `std::nullopt` en fallo).
- `executeQuery(query, params={})` — SELECT, devuelve `std::vector<std::vector<SqlParam>>` (una fila = un vector de columnas).

`SqlParam` es `std::variant<int, long long, std::string>`, mapeado a los tipos reales de columna de `SQL/init.sql` (`INT`, `BIGINT`, `VARCHAR`/`DATETIME`-como-texto) vía `std::visit`.

# Why it matters

Es la única forma en que el backend habla con MySQL; cualquier query nueva pasa por acá.

# Best Practices

- Siempre usar `executePrepared`/`executeQuery` (prepared statements) para evitar inyección SQL — nunca concatenar strings en una query.
- Llamar `mysql_library_init(0, nullptr, nullptr)` explícitamente antes de cualquier otra función de la API (ver "gotcha" abajo) — ya resuelto una única vez por proceso en `DatabaseManager` vía `std::call_once`.

# Common Mistakes

- **Bindear resultados de columnas `DATETIME`/`TIMESTAMP` con su tipo nativo de MySQL.** El cliente escribe un `struct MYSQL_TIME` binario en el buffer en vez de texto; leerlo como `std::string(buffer, length)` da basura, no `"2026-07-28 10:00:00"`. `executeQuery` fuerza `buffer_type = MYSQL_TYPE_STRING` para *todas* las columnas al bindear resultados (MySQL convierte a texto del lado del servidor), y usa el tipo original de la columna (`campos[i].type`, guardado aparte) solo para decidir si conviene parsear ese texto a `int`/`long long`.
- **Gotcha de plataforma (MinGW)**: en el build estático de MinGW, el constructor global que normalmente inicializa la librería (WSAStartup, etc.) no corre solo de forma confiable. Sin `mysql_library_init()` explícito, la conexión falla con `Lost connection to server at 'handshake: reading initial communication packet'` aunque el servidor esté perfectamente disponible — un error que no menciona para nada la causa real, difícil de diagnosticar sin saber de este gotcha.
- Olvidar el `return` antes de un `std::nullopt` en un branch de error: el bug real que se dio en este proyecto fue que dos branches de error de `executePrepared` hacían `std::nullopt;` como sentencia suelta (sin `return`), así que el control seguía y la función terminaba devolviendo `0` (éxito) incluso cuando el INSERT había fallado.

# Hermes Usage

Ver [[ADR-008 - MySQL como Base de Datos]] y [[ADR-009 - libmariadb como Cliente MySQL]] para el contexto completo de la decisión, y `docs/API_REQUIREMENTS.md` para los endpoints reales que usan `DatabaseManager`.

# Related Notes

- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[RAII]]
- [[MariaDB Connector-C - Documentacion Oficial]]

# References

- docs/decisions/0001-cliente-base-de-datos.md (Priority 1)
- https://mariadb.com/kb/en/mariadb-connector-c/ (Priority 2)
