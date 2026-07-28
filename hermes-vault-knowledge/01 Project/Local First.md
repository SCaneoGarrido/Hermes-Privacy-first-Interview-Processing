---
title: Local First
aliases: []
tags: [project, principle, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: .ai/PROJECT.md
related: ["Privacy First", "Hermes - Vision General", "Stack Tecnologico de Hermes", "ADR-010 - Docker Compose para MySQL Local"]
---

# Summary

Principio inmutable de Hermes: la plataforma debe funcionar por completo sin acceso a Internet.

# Explanation

Local First significa que ninguna funcionalidad crítica (transcripción, anonimización, generación de resúmenes, exportación) depende de conectividad externa. Esto se traduce técnicamente en la elección de motores locales: [[whisper.cpp - Repositorio Oficial|whisper.cpp]] para reconocimiento de voz y [[Ollama - Documentacion Oficial|Ollama]] como motor de LLM, ambos ejecutándose en la máquina del usuario.

# Why it matters

Un investigador debe poder usar Hermes en un entorno sin Internet (por ejemplo, en una institución con políticas de red restrictivas) sin perder funcionalidad. Local First es también un prerequisito técnico de [[Privacy First]]: si los datos no salen de la máquina, es porque el procesamiento tampoco depende de un servidor remoto.

# Best Practices

- Cualquier feature nueva debe poder probarse con la red deshabilitada.
- Preferir binarios locales (whisper.cpp, Ollama) sobre clientes de servicios remotos.

# Common Mistakes

- Introducir una dependencia "opcional" a un servicio en la nube que silenciosamente rompe el modo offline.

# Hermes Usage

Local First fundamenta [[ADR-004 - whisper.cpp para Reconocimiento de Voz]] y [[ADR-005 - Ollama como Motor LLM]]. La base de datos es la excepción matizada: [[ADR-008 - MySQL como Base de Datos|MySQL]] corre localmente vía [[ADR-010 - Docker Compose para MySQL Local|Docker]], así que sigue sin depender de Internet ni de un servicio externo, pero sí requiere un proceso de servidor local — a diferencia de SQLite (embebido, sin servidor), que era la elección original ([[ADR-006 - SQLite como Base de Datos]], superada).

# Related Notes

- [[Privacy First]]
- [[Hermes - Vision General]]
- [[Stack Tecnologico de Hermes]]

# References

- .ai/PROJECT.md (Priority 1)
