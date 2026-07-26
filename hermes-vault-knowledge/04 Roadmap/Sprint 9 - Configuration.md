---
title: Sprint 9 - Configuration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 8 - Frontend", "Sprint 10 - Testing"]
---

# Summary

Configuración completa de modelos, almacenamiento, idioma, puertos y LLM.

# Explanation

**Objetivo:** configuración completa.

**Entregables:** configuración de modelos, configuración de almacenamiento, configuración de idioma, configuración de puertos, configuración de LLM.

# Why it matters

Permite a cada investigador adaptar Hermes a su hardware y necesidades (por ejemplo, elegir un modelo de whisper.cpp o de Ollama más liviano) sin tocar código.

# Best Practices

- Centralizar la configuración en un único módulo/archivo, evitando parámetros dispersos por el código (.ai/AI_INSTRUCTIONS.md: "Keep classes cohesive").

# Common Mistakes

- Hardcodear rutas de almacenamiento o puertos en vez de exponerlos como configuración.

# Hermes Usage

Prepara el terreno para la fase de calidad en [[Sprint 10 - Testing]].

# Related Notes

- [[Sprint 8 - Frontend]]
- [[Sprint 10 - Testing]]

# References

- README.md (Priority 2)
