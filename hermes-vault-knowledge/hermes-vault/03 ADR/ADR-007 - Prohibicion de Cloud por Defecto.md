---
title: ADR-007 - Prohibicion de Cloud por Defecto
aliases: ["Cloud Forbidden by Default"]
tags: [adr, hermes, privacy]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Privacy First", "Local First", "Alcance y Exclusiones"]
---

# Context

Las entrevistas procesadas por Hermes pueden contener información médica o de investigación altamente sensible.

# Decision

Prohibir por defecto cualquier procesamiento en la nube.

# Alternatives

Permitir integraciones cloud opcionales configurables por el usuario: no adoptado como comportamiento por defecto; cualquier excepción requeriría aprobación explícita (ver [[Alcance y Exclusiones]]).

# Consequences

- Ninguna función del sistema puede depender de un servicio externo para operar correctamente.
- Toda nueva dependencia de red debe justificarse explícitamente y no puede activarse por defecto.
- Refuerza directamente las decisiones [[ADR-004 - whisper.cpp para Reconocimiento de Voz]] y [[ADR-005 - Ollama como Motor LLM]].

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- [[Privacy First]]
