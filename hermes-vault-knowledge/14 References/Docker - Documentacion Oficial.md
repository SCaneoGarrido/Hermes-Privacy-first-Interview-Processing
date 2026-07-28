---
title: Docker - Documentacion Oficial
aliases: []
tags: [reference, docker, deployment]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: https://docs.docker.com/
related: ["ADR-010 - Docker Compose para MySQL Local", "ADR-008 - MySQL como Base de Datos"]
---

# Summary

Documentación oficial de Docker y Docker Compose, usados para correr MySQL localmente en Hermes ([[ADR-010 - Docker Compose para MySQL Local]]).

# Explanation

Fuentes oficiales:
- https://docs.docker.com/
- https://docs.docker.com/compose/ (Compose específicamente)
- https://hub.docker.com/_/mysql (imagen oficial usada en `docker-compose.yml`)

# Why it matters

Es la única pieza de infraestructura de Hermes que corre fuera del propio ejecutable de C++; su configuración (`docker-compose.yml`, `.env`) determina cómo se conecta `DatabaseManager` a MySQL en desarrollo.

# Best Practices

- Bindear puertos solo a `127.0.0.1`, nunca `0.0.0.0`, para no exponer la base de datos en la red local ([[Privacy First]]).
- Mantener `.env` fuera de git (`.gitignore`); versionar solo `.env.example` con placeholders.

# Common Mistakes

- Cambiar credenciales en `.env` después de que el volumen de datos ya se inicializó: el script de init de la imagen de MySQL solo corre la primera vez que el volumen está vacío, así que el usuario/password real sigue siendo el original hasta que se recree el volumen (`docker compose down -v`).

# Hermes Usage

**Tensión abierta**: README.md lista "Docker obligatorio" bajo "Fuera del alcance (v1.0)", pero hoy es un requisito real para levantar la base de datos local — ver [[ADR-010 - Docker Compose para MySQL Local]] para el detalle de esta tensión sin resolver.

# Related Notes

- [[ADR-010 - Docker Compose para MySQL Local]]
- [[ADR-008 - MySQL como Base de Datos]]

# References

- https://docs.docker.com/ (Priority 2)
- https://docs.docker.com/compose/ (Priority 2)
