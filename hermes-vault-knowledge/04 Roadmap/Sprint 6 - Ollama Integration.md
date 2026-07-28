---
title: Sprint 6 - Ollama Integration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 5 - Whisper Integration", "Sprint 7 - Export", "ADR-005 - Ollama como Motor LLM", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama", "Ollama Integration Strategy"]
---

# Summary

Integración de IA local (Ollama) para corrección ortográfica, puntuación, resúmenes y anonimización.

# Explanation

**Objetivo:** integración IA local.

**Entregables:** cliente HTTP hacia Ollama, corrección ortográfica, puntuación, resúmenes, anonimización.

> **Diseño escrito el 2026-07-28** en [[Ollama Integration Strategy]] (`status: draft`) y [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]] (`status: proposed`), pendientes de aprobación. Informado por datos reales de Sprint 5 (entrevista de ~70 min, 1804 segmentos): ningún prompt único a Ollama entra en ventana de contexto, así que el diseño resuelve chunking con estrategias distintas por tarea (corrección/estructuración por bloques con continuidad, anonimización con tabla de sustitución de dos pasadas para consistencia entre bloques, resumen con map-reduce). Anonimización queda marcada como el entregable crítico — bloquea [[Sprint 7 - Export]].

# Why it matters

La anonimización es el entregable más directamente ligado al principio [[Privacy First]]: convierte una transcripción cruda en un documento seguro de compartir.

# Best Practices

- Aislar el cliente de Ollama detrás de `ILLMClient` (ver [[Filosofia de Repositorios]]) para poder cambiar de modelo sin tocar el dominio.

# Common Mistakes

- Asumir que la anonimización del LLM es 100% infalible sin validación o revisión adicional antes de exportar.

# Hermes Usage

Cierra el pipeline de procesamiento de entrevistas antes de la exportación en [[Sprint 7 - Export]].

# Related Notes

- [[Sprint 5 - Whisper Integration]]
- [[Sprint 7 - Export]]
- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]
- [[Ollama Integration Strategy]]

# References

- README.md (Priority 2)
