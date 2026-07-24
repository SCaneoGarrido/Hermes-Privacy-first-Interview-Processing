---
title: Ollama - Documentacion Oficial
aliases: ["Ollama"]
tags: [reference, ollama, ai, pending]
status: pending
created: 2026-07-24
updated: 2026-07-24
source: 
related: ["ADR-005 - Ollama como Motor LLM", "08 AI"]
---

# Summary

Nota de referencia pendiente: Ollama es el motor de LLM de Hermes, pero su documentación oficial todavía no está registrada en `.ai/OFFICIAL_SOURCES.md`.

# Explanation

`.ai/OFFICIAL_SOURCES.md` (Priority 1) lista fuentes oficiales para CMake, Crow, SQLite, whisper.cpp, React, Vite y Node.js, pero **no incluye una URL oficial de Ollama**, a pesar de que Ollama es una decisión de arquitectura ya aceptada ([[ADR-005 - Ollama como Motor LLM]]).

Esta nota existe para: (1) resolver los enlaces internos hacia "Ollama" desde otras notas, y (2) dejar constancia explícita del vacío en las fuentes oficiales, en vez de asumir o inventar una URL sin confirmación.

# Why it matters

Sin una fuente oficial registrada, cualquier nota futura sobre la API de Ollama (ver [[08 AI]], tema pendiente "Ollama API") carecería de una referencia de Priority 2 verificable.

# Best Practices

Antes de documentar la API de Ollama en profundidad, confirmar y añadir la URL oficial a `.ai/OFFICIAL_SOURCES.md`.

# Common Mistakes

Asumir una URL de memoria y tratarla como fuente oficial sin verificarla contra `.ai/OFFICIAL_SOURCES.md`.

# Hermes Usage

Ver [[ADR-005 - Ollama como Motor LLM]] para la decisión de arquitectura ya aceptada; esta nota solo cubre el vacío documental de la fuente oficial.

# Related Notes

- [[ADR-005 - Ollama como Motor LLM]]
- [[08 AI]]

# References

- .ai/OFFICIAL_SOURCES.md (Priority 1 — Ollama ausente, acción pendiente)
