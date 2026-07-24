---
title: Sprint 7 - Export
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 6 - Ollama Integration", "Sprint 8 - Frontend"]
---

# Summary

Exportación de entrevistas procesadas en múltiples formatos.

# Explanation

**Objetivo:** exportación.

**Entregables:** TXT, DOCX, PDF, JSON.

# Why it matters

Es el entregable final visible para el investigador: el resultado tangible del pipeline de transcripción y anonimización.

# Best Practices

- Mantener los exportadores como adaptadores independientes (uno por formato) detrás de una interfaz común.

# Common Mistakes

- Generar el DOCX/PDF acoplado directamente al modelo de dominio en vez de a un DTO de exportación.

# Hermes Usage

Cierra el pipeline funcional del backend antes de construir la interfaz web en [[Sprint 8 - Frontend]].

# Related Notes

- [[Sprint 6 - Ollama Integration]]
- [[Sprint 8 - Frontend]]

# References

- README.md (Priority 2)
