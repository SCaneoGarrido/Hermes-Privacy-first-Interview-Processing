---
title: Privacy First
aliases: []
tags: [project, principle, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Local First", "Hermes - Vision General", "ADR-007 - Prohibicion de Cloud por Defecto"]
---

# Summary

Principio inmutable de Hermes: la información sensible de una entrevista nunca debe salir de la máquina local, salvo que el usuario lo configure explícitamente.

# Explanation

Privacy First es el primero de los principios inmutables del proyecto. Implica que ningún componente del sistema puede enviar datos de entrevistas (audio, transcripciones, metadatos) a un servicio externo por defecto. Cualquier excepción requiere una configuración explícita y consciente por parte del usuario, no un comportamiento implícito o por defecto.

# Why it matters

Las entrevistas de investigación cualitativa pueden contener datos de pacientes, profesionales de la salud o identificadores personales. Un fallo en este principio no es un bug técnico: es una violación de privacidad con consecuencias éticas y legales.

# Best Practices

- Toda nueva dependencia de red debe justificarse explícitamente (ver .ai/AI_INSTRUCTIONS.md: "Always explain architectural decisions when introducing a new dependency").
- Verificar que ninguna librería de terceros telemetríe datos sin consentimiento.

# Common Mistakes

- Añadir SDKs de servicios cloud (OpenAI API, Azure, AWS, Google Cloud) como dependencia por comodidad: están explícitamente fuera de alcance (ver [[Alcance y Exclusiones]]).

# Hermes Usage

Este principio es la razón de ser de decisiones como [[ADR-004 - whisper.cpp para Reconocimiento de Voz]] (transcripción local en vez de un servicio de speech-to-text en la nube) y [[ADR-005 - Ollama como Motor LLM]] (LLM local en vez de una API de terceros).

# Related Notes

- [[Local First]]
- [[Hermes - Vision General]]
- [[ADR-007 - Prohibicion de Cloud por Defecto]]
- [[Alcance y Exclusiones]]

# References

- .ai/PROJECT.md (Priority 1)
