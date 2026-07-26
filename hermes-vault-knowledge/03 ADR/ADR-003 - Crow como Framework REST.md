---
title: ADR-003 - Crow como Framework REST
aliases: []
tags: [adr, hermes, frameworks]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["API First", "Stack Tecnologico de Hermes"]
---

# Context

Hermes necesita exponer su lógica de negocio mediante una REST API en C++ (ver [[API First]]).

# Decision

Usar **Crow** como framework REST.

# Alternatives

No se documentan alternativas explícitas en .ai/DECISIONS.md.

# Consequences

- Framework minimalista, similar en filosofía a Express (Node.js) o Flask (Python), lo que reduce la curva de aprendizaje.
- Crow queda confinado a la capa `REST API`; el dominio no debe depender de sus tipos (ver [[Clean Architecture]]).

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- https://crowcpp.org/master/ (Priority 2)
- https://github.com/crowcpp/crow (Priority 4)
