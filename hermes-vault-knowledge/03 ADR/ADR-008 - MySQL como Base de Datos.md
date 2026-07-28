---
title: ADR-008 - MySQL como Base de Datos
aliases: []
tags: [adr, hermes, libraries]
status: accepted
created: 2026-07-28
updated: 2026-07-28
source: .ai/DECISIONS.md
related: ["ADR-006 - SQLite como Base de Datos", "ADR-009 - libmariadb como Cliente MySQL", "ADR-010 - Docker Compose para MySQL Local", "Stack Tecnologico de Hermes", "Local First"]
---

# Context

El proyecto había arrancado pensando en SQLite ([[ADR-006 - SQLite como Base de Datos]]), pero se decidió migrar a una base de datos robusta desde el día uno en vez de migrar más adelante, cuando ya hubiera código y datos reales dependiendo de SQLite. En el momento de la decisión no existía todavía ningún código de acceso a datos, así que el cambio no implicó migración de datos ni de código existente.

# Decision

Usar **MySQL**, corriendo en Docker (ver [[ADR-010 - Docker Compose para MySQL Local]]), como base de datos de Hermes. Reemplaza a [[ADR-006 - SQLite como Base de Datos]].

# Alternatives

- Mantener SQLite y migrar más adelante: descartado porque migrar sin código ni datos de por medio es gratis, y hacerlo después no lo sería.
- PostgreSQL: sigue fuera de alcance para v1.0 (README.md, "Fuera del alcance").

# Consequences

- Deja de cumplirse el motivo original de [[ADR-006 - SQLite como Base de Datos]] ("no requiere proceso de servidor independiente"): MySQL sí requiere un servidor, corriendo en Docker.
- **Tensión con el alcance declarado**: README.md lista "Docker obligatorio" bajo "Fuera del alcance (v1.0)", pero en la práctica Docker es hoy un requisito real para levantar la base de datos local — no existe una ruta de instalación nativa documentada. Esto queda registrado como una tensión abierta entre esta ADR y el alcance de v1.0, no resuelta unilateralmente por esta nota.
- El acceso a MySQL debe seguir quedando oculto detrás de repositorios según [[Filosofia de Repositorios]]. **Desviación conocida (2026-07-28)**: hoy `InterviewController` y `FileController` llaman directamente a `DatabaseManager::getInstance()`, sin una interfaz de repositorio intermedia — ver [[Sprint 2 - Persistence]] para el detalle de esta deuda técnica.
- Habilita [[ADR-009 - libmariadb como Cliente MySQL]] (qué librería cliente C++ usar) y [[ADR-011 - Contrato de Respuesta API Uniforme]] (cómo se exponen los resultados vía REST).

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- docs/decisions/0001-cliente-base-de-datos.md (Priority 1 — writeup completo de esta decisión y la de [[ADR-009 - libmariadb como Cliente MySQL]])
- https://dev.mysql.com/doc/refman/en/ (Priority 2)
