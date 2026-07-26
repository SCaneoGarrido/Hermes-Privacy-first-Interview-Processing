---
title: ADR-005 - Ollama como Motor LLM
aliases: []
tags: [adr, hermes, ai]
status: accepted
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["Local First", "Privacy First", "Stack Tecnologico de Hermes"]
---

# Context

Hermes necesita un motor de LLM para corrección ortográfica, puntuación, resúmenes y anonimización de entrevistas (Sprint 6), sin exponer el contenido a un proveedor externo.

# Decision

Usar **Ollama** como motor de ejecución de LLM, con **Qwen** como modelo por defecto.

# Alternatives

No se documentan alternativas explícitas en .ai/DECISIONS.md; APIs de LLM en la nube (OpenAI API) quedan descartadas por principio (ver [[Alcance y Exclusiones]]).

# Consequences

- Ejecución 100% offline del LLM.
- El acceso a Ollama queda detrás de una interfaz `ILLMClient` (ver [[Filosofia de Repositorios]]), permitiendo cambiar de modelo o motor sin afectar el dominio.
- El rendimiento y calidad de las respuestas dependen del hardware local del usuario y del modelo Qwen elegido.

# Status

Accepted

# References

- .ai/DECISIONS.md (Priority 1)
- .ai/PROJECT.md (Priority 1)
