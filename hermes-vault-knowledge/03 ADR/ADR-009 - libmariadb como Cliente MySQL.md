---
title: ADR-009 - libmariadb como Cliente MySQL
aliases: ["MariaDB Connector/C"]
tags: [adr, hermes, libraries]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: .ai/DECISIONS.md
related: ["ADR-008 - MySQL como Base de Datos", "MariaDB Connector-C (libmariadb)", "Hermes Coding Standard"]
---

# Context

Elegido MySQL como base de datos ([[ADR-008 - MySQL como Base de Datos]]), faltaba decidir con qué librería cliente C++ conectarse desde Crow. Se evaluó **mysql-connector-cpp** (el connector oficial de Oracle/MySQL) contra **libmariadb** (MariaDB Connector/C).

El toolchain de Hermes en esta máquina no tiene Visual Studio instalado; compila con MinGW-w64 vía vcpkg con el triplet `x64-mingw-static`.

# Decision

Usar **libmariadb** (paquete `libmariadb` de vcpkg, target de CMake `unofficial::libmariadb`), no mysql-connector-cpp.

# Alternatives

- **mysql-connector-cpp con el feature `jdbc`** (API relacional clásica, puerto 3306): descartado — depende del puerto `libmysql` de vcpkg, que declara explícitamente `"supports": "!android & !mingw & !uwp & !xbox"`. La build falla en `vcpkg install` bajo MinGW.
- **mysql-connector-cpp sin `jdbc` (X DevAPI)**: sí compila en MinGW, pero descartado por dos motivos: (1) usa el protocolo X (puerto 33060 por defecto) en vez del protocolo clásico de MySQL, lo que exige exponer y documentar un puerto extra en Docker; (2) arrastra una cadena de dependencias pesada (protobuf, openssl, rapidjson, zlib, lz4, zstd) con tiempos de compilación largos.

# Consequences

- `libmariadb` solo depende de `zlib` (+ iconv en Windows) — build mucho más rápido, relevante para un proyecto que quiere ser fácil de clonar y compilar por colaboradores externos (ver [[Filosofia de Repositorios|Open Source]]).
- Habla el protocolo clásico de MySQL (puerto 3306), sin puertos ni plugins adicionales que documentar en Docker.
- **Gotcha de plataforma descubierto en la práctica**: en el build estático de MinGW, el constructor global que normalmente inicializa la librería (WSAStartup, etc.) no corre solo de forma confiable. Sin llamar explícitamente a `mysql_library_init(0, nullptr, nullptr)` antes de cualquier otra función de la API, la conexión falla con `Lost connection to server at 'handshake: reading initial communication packet'` aunque el servidor esté perfectamente disponible. `DatabaseManager` lo invoca una única vez por proceso vía `std::call_once`.
- Es API en C (no C++ nativo): `DatabaseManager` envuelve `MYSQL*`/`MYSQL_STMT*` con RAII (`std::unique_ptr` + deleters custom) en vez de depender de una envoltura C++ propia del connector.

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- docs/decisions/0001-cliente-base-de-datos.md (Priority 1 — incluye la nota de compatibilidad MinGW)
- https://mariadb.com/kb/en/mariadb-connector-c/ (Priority 2)
