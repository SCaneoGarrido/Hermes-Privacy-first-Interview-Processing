---
title: Sprint 4 - Background Processing
aliases: []
tags: [roadmap, sprint, hermes]
status: done
created: 2026-07-24
updated: 2026-08-15
source: README.md
related: ["Sprint 3 - File Upload", "Sprint 5 - Whisper Integration", "Thread Pools"]
---

# Summary

Procesamiento asíncrono de entrevistas mediante una cola de trabajos y worker threads.

# Explanation

**Objetivo:** procesamiento asíncrono.

**Entregables:** Job Queue, Worker Threads, estados, progress tracking.

**Estados:** Pending, Running, Completed, Failed.

> **Implementado.** Cola de trabajos in-memory thread-safe (`queue` + `mutex` + `condition_variable`) y worker pool de N threads RAII (`WORKER_POOL_SIZE`, default 1): dequeue → `markRunning` → `handler.execute` → `markCompleted`/`markFailed`. Máquina de estados `interview_jobs` (pending/running/completed/failed) persistida, con historial de intentos por entrevista en vez de `UNIQUE(interview_id)`, para permitir reintentos como filas nuevas. Dedupe: si ya hay un job activo para la entrevista, `POST /interview/:id/process` responde 409 `JOB_ALREADY_QUEUED` en vez de encolar dos veces. Recuperación de crashes: `reclaimStuckJobs()` corre al arrancar el backend, antes de aceptar tráfico, y marca como `failed` cualquier job que haya quedado `running` de una corrida anterior interrumpida. De paso, cerró una condición de carrera que existía desde [[Sprint 0 - Foundation]]: se serializó `executeQuery`/`executePrepared` sobre la única conexión MySQL con un mutex, porque antes de este sprint el acceso concurrente a `DatabaseManager` no estaba protegido.

# Why it matters

La transcripción con whisper.cpp y el procesamiento con Ollama son operaciones costosas; deben ejecutarse fuera del hilo que atiende peticiones REST para no bloquear la API.

# Best Practices

- Diseñar la máquina de estados (Pending/Running/Completed/Failed) de forma explícita y persistida, no solo en memoria.

# Common Mistakes

- Compartir estado mutable entre worker threads sin sincronización adecuada.

# Hermes Usage

Este sprint introduce la necesidad del concepto [[Thread Pools]] (pendiente de documentar, ver Knowledge Backlog) y habilita [[Sprint 5 - Whisper Integration]] y [[Sprint 6 - Ollama Integration]] sin bloquear la API REST. Es el módulo `backend/jobs/` (`JobQueue`, `WorkerPool`, `InterviewJobRepository`).

# Related Notes

- [[Sprint 3 - File Upload]]
- [[Sprint 5 - Whisper Integration]]

# References

- README.md (Priority 2)
