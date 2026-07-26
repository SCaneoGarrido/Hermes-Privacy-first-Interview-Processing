---
title: ADR-006 - SQLite como Base de Datos
aliases: []
tags: [adr, hermes, libraries]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Local First", "Stack Tecnologico de Hermes", "Filosofia de Repositorios"]
---

# Context

Hermes necesita persistir metadatos de entrevistas, transcripciones y configuración, sin requerir infraestructura de servidor.

# Decision

Usar **SQLite** como base de datos.

# Alternatives

- PostgreSQL: descartado explícitamente, fuera de alcance para v1.0 (README.md).
- No se documentan otras alternativas en .ai/DECISIONS.md.

# Consequences

- No requiere proceso de servidor independiente, coherente con [[Local First]].
- El acceso a SQLite debe quedar oculto detrás de repositorios (ver [[Filosofia de Repositorios]]); ningún controller accede a SQLite directamente.
- Limitaciones de concurrencia de escritura propias de SQLite deben considerarse en el diseño de Sprint 4 (Background Processing / Worker Threads).

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- https://sqlite.org/docs.html (Priority 2)
- https://sqlite.org/c3ref/intro.html (Priority 2)
