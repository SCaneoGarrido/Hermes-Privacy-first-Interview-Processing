---
title: ADR-001 - Modular Monolith
aliases: []
tags: [adr, architecture, hermes]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Modular Monolith", "Arquitectura en Capas de Hermes"]
---

# Context

Hermes necesita una arquitectura de despliegue. Las alternativas típicas son microservicios, monolito tradicional (no modular) o monolito modular.

# Decision

Adoptar un **Modular Monolith**: un único ejecutable compuesto por módulos internos independientes.

# Alternatives

- Microservicios: descartado, fuera de alcance del proyecto (ver [[Alcance y Exclusiones]]).
- Monolito no modular: descartado por riesgo de acoplamiento excesivo a largo plazo.

# Consequences

- Despliegue simple: un solo binario, sin orquestación.
- Requiere disciplina interna para mantener módulos desacoplados (ver [[Arquitectura en Capas de Hermes]]).
- Alineado con la meta de instalación en menos de 10 minutos.

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- [[Modular Monolith]]
