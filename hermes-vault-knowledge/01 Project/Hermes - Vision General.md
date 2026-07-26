---
title: Hermes - Vision General
aliases: ["Hermes Overview"]
tags: [project, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Privacy First", "Local First", "API First", "Modular Monolith", "Clean Architecture", "Stack Tecnologico de Hermes", "Roadmap General de Hermes"]
---

# Summary

Hermes es una plataforma open-source para el procesamiento local de entrevistas de investigación cualitativa: transcripción, anonimización y generación de resúmenes, ejecutando toda la inteligencia artificial en la máquina del usuario.

# Explanation

El sistema atiende un escenario de investigación cualitativa donde las entrevistas pueden contener información altamente sensible (datos de pacientes, profesionales de la salud, identificadores personales). Por ello Hermes adopta una filosofía estricta **Local First**: no requiere servicios cloud ni APIs externas, y todos los modelos de IA (transcripción y LLM) se ejecutan localmente.

El flujo de valor que ofrece a un investigador es:

1. Subir una entrevista (audio).
2. Transcribir localmente el audio.
3. Anonimizar información sensible.
4. Generar resúmenes.
5. Exportar documentos.

Todo esto sin que la información confidencial salga de la máquina local.

# Why it matters

Un investigador que trabaja con entrevistas médicas o sociales no puede subir grabaciones a servicios cloud de transcripción sin comprometer la privacidad de los participantes. Hermes existe para eliminar esa disyuntiva entre "usar IA" y "proteger la privacidad".

# Best Practices

- Cualquier funcionalidad nueva debe evaluarse primero contra los principios inmutables del proyecto (ver [[Privacy First]], [[Local First]]).
- Toda lógica de negocio debe exponerse vía REST ([[API First]]), nunca acoplada directamente al frontend.

# Common Mistakes

- Asumir que una integración cloud "opcional" es aceptable sin aprobación explícita: está fuera de alcance por defecto (ver [[ADR-007 - Prohibicion de Cloud por Defecto]]).
- Confundir Hermes con una arquitectura de microservicios: es un **Modular Monolith** (ver [[Modular Monolith]]).

# Hermes Usage

Esta nota es la raíz conceptual del proyecto: toda decisión de arquitectura, roadmap o dependencia debe poder trazarse hasta la misión aquí descrita.

# Related Notes

- [[Privacy First]]
- [[Local First]]
- [[API First]]
- [[Maintainability over Cleverness]]
- [[Stack Tecnologico de Hermes]]
- [[Filosofia de Repositorios]]
- [[Alcance y Exclusiones]]
- [[Roadmap General de Hermes]]

# References

- .ai/PROJECT.md (Priority 1)
