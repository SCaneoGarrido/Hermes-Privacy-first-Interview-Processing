---
title: ADR-004 - whisper.cpp para Reconocimiento de Voz
aliases: []
tags: [adr, hermes, ai]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Local First", "Privacy First", "Stack Tecnologico de Hermes"]
---

# Context

Hermes necesita transcribir audio de entrevistas localmente, sin depender de un servicio cloud de speech-to-text, en cumplimiento de [[Local First]] y [[Privacy First]].

# Decision

Usar **whisper.cpp** para reconocimiento de voz.

# Alternatives

No se documentan alternativas explícitas en .ai/DECISIONS.md; servicios cloud de transcripción quedan descartados por principio (ver [[Alcance y Exclusiones]]).

# Consequences

- Implementación nativa en C++, integrable directamente en el backend sin proceso intermedio.
- La transcripción de audio queda confinada a la capa `Infrastructure`, detrás de una interfaz `ITranscriber` (ver [[Filosofia de Repositorios]]).
- Requiere gestionar localmente los modelos de whisper (descarga y almacenamiento local de pesos).

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- https://github.com/ggml-org/whisper.cpp (Priority 4)
