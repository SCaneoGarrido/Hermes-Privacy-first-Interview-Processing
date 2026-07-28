---
title: ADR-010 - Docker Compose para MySQL Local
aliases: []
tags: [adr, hermes, deployment]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: .ai/DECISIONS.md
related: ["ADR-008 - MySQL como Base de Datos", "Local First", "Alcance y Exclusiones"]
---

# Context

Elegido MySQL ([[ADR-008 - MySQL como Base de Datos]]), había que decidir cómo corre localmente en la máquina de cada desarrollador: instalado nativo, o en un contenedor Docker.

# Decision

Usar **Docker Compose** (`docker-compose.yml` en la raíz del repo) para correr MySQL localmente, no una instalación nativa.

# Alternatives

- **Instalar MySQL nativo** en cada máquina de desarrollo: descartado. Desarrollar contra un MySQL nativo y containerizar recién al final arriesga diferencias sutiles (versión de MySQL, charset/collation por defecto, plugin de autenticación) que aparecen tarde, cuando ya hay código y datos de prueba dependiendo de ese comportamiento.

# Consequences

- Todo el equipo levanta la misma versión de MySQL con `docker compose up`, sin pelear con versiones distintas entre máquinas.
- El puerto de MySQL se bindea solo a `127.0.0.1` en `docker-compose.yml` (no `0.0.0.0`), coherente con [[Privacy First]]: la base no debe quedar accesible desde la red local aunque el desarrollador se olvide.
- **Tensión con el alcance declarado (sin resolver)**: README.md lista "Docker obligatorio" bajo "Fuera del alcance (v1.0)". En la práctica, hoy Docker es un requisito real — no hay una ruta de instalación nativa documentada ni soportada. Esta nota deja constancia de la tensión; resolverla (documentar una alternativa nativa, o actualizar el alcance declarado) es una decisión de producto pendiente, no algo que esta ADR zanje por sí sola.
- Herramientas cliente (MySQL Workbench, etc.) se conectan igual por `127.0.0.1:3306` sin importar si el servidor corre nativo o en contenedor — no hace falta instalar el servidor MySQL en la máquina, solo un cliente.

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- docker-compose.yml (raíz del repositorio)
- README.md, sección "Fuera del alcance (v1.0)" (Priority 2 — fuente de la tensión señalada arriba)
