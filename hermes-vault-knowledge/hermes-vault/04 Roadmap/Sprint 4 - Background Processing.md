---
title: Sprint 4 - Background Processing
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 3 - File Upload", "Sprint 5 - Whisper Integration", "Thread Pools"]
---

# Summary

Procesamiento asíncrono de entrevistas mediante una cola de trabajos y worker threads.

# Explanation

**Objetivo:** procesamiento asíncrono.

**Entregables:** Job Queue, Worker Threads, estados, progress tracking.

**Estados:** Pending, Running, Completed, Failed.

# Why it matters

La transcripción con whisper.cpp y el procesamiento con Ollama son operaciones costosas; deben ejecutarse fuera del hilo que atiende peticiones REST para no bloquear la API.

# Best Practices

- Diseñar la máquina de estados (Pending/Running/Completed/Failed) de forma explícita y persistida, no solo en memoria.

# Common Mistakes

- Compartir estado mutable entre worker threads sin sincronización adecuada.

# Hermes Usage

Este sprint introduce la necesidad del concepto [[Thread Pools]] (pendiente de documentar, ver Knowledge Backlog) y habilita [[Sprint 5 - Whisper Integration]] y [[Sprint 6 - Ollama Integration]] sin bloquear la API REST.

# Related Notes

- [[Sprint 3 - File Upload]]
- [[Sprint 5 - Whisper Integration]]

# References

- README.md (Priority 2)
