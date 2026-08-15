---
title: Sprint 7 - Export
aliases: []
tags: [roadmap, sprint, hermes]
status: draft (bloqueado, no iniciado)
created: 2026-07-24
updated: 2026-08-15
source: README.md
related: ["Sprint 6 - Ollama Integration", "Sprint 8 - Frontend"]
---

# Summary

Exportación de entrevistas procesadas en múltiples formatos.

# Explanation

**Objetivo:** exportación.

**Entregables:** TXT, DOCX, PDF, JSON.

> **No iniciado, bloqueado a propósito.** [[Sprint 6 - Ollama Integration]] dejó documentado que el recall de la anonimización automática es incompleto (nombres de figuras públicas mencionadas de pasada no siempre se detectan). Exportar por defecto sin resolver eso primero rompería la promesa de [[Privacy First]]. Cuando se implemente, este sprint necesita más que un chequeo booleano de "¿corrió la anonimización sin error?" — probablemente una advertencia explícita al usuario de que el resultado amerita revisión antes de compartirlo, hasta que el recall mejore.

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
