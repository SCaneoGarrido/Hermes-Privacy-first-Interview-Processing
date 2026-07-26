---
title: Ollama - Documentacion Oficial
aliases: ["Ollama"]
tags: [reference, ollama, ai]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://ollama.com/
related: ["ADR-005 - Ollama como Motor LLM", "08 AI"]
---

# Summary

Documentación oficial de Ollama, el motor de LLM local de Hermes. Fuente confirmada el 2026-07-24 vía `docs/documentation_tech.yml`, que cerró el vacío detectado previamente en `.ai/OFFICIAL_SOURCES.md` (no listaba ninguna URL de Ollama).

# Explanation

Fuentes oficiales:
- https://ollama.com/ — sitio oficial: permite ejecutar modelos de lenguaje abiertos localmente ("run any app or agent with open models"), con instalación simple y ejecución 100% local; opcionalmente ofrece un servicio cloud para modelos más grandes, sin entrenar con los datos del usuario.
- https://github.com/ollama/ollama — repositorio oficial.
- https://github.com/ollama/ollama/blob/main/docs/api.md — referencia de la API REST.

**Endpoints principales de la API REST** (verificado 2026-07-24):
- `POST /api/generate` — completado de texto, con streaming.
- `POST /api/chat` — conversación multi-turno con historial de mensajes.
- `GET /api/tags` — listar modelos locales.
- `POST /api/pull` — descargar un modelo.
- `POST /api/create` — construir un modelo personalizado (por ejemplo, un Modelfile para Qwen ajustado a Hermes).
- `POST /api/embed` — generar embeddings.
- `DELETE /api/delete` — eliminar un modelo.

Las peticiones incluyen el nombre del modelo y parámetros específicos (`prompt` para generación, `messages` para chat); las respuestas son JSON y soportan streaming por defecto (desactivable con `"stream": false"`), incluyendo métricas de generación (conteo de tokens, duración).

# Why it matters

Esta es la fuente de Priority 2 que sustenta [[ADR-005 - Ollama como Motor LLM]] y habilita el desarrollo real de la nota pendiente "Ollama API" en [[08 AI]]: ya no depende de una URL sin confirmar.

# Best Practices

Actualizar `.ai/OFFICIAL_SOURCES.md` para incluir estas URLs (acción sugerida al equipo, no ejecutada por el curador ya que ese archivo es Priority 1 y no forma parte del alcance de esta bóveda).

# Common Mistakes

Documentar la API de Ollama sin distinguir entre `/api/generate` (completado simple) y `/api/chat` (con historial), lo que puede llevar a un diseño incorrecto de `ILLMClient` (ver [[Filosofia de Repositorios]]).

# Hermes Usage

`ILLMClient`/`OllamaClient` (ver [[Filosofia de Repositorios]]) debería mapear directamente a `/api/chat` para las tareas de Sprint 6 (corrección, puntuación, resúmenes, anonimización), dado que son tareas conversacionales con contexto de la transcripción completa.

# Related Notes

- [[ADR-005 - Ollama como Motor LLM]]
- [[08 AI]]
- [[Filosofia de Repositorios]]

# References

- https://ollama.com/ (Priority 2, docs/documentation_tech.yml)
- https://github.com/ollama/ollama (Priority 4, docs/documentation_tech.yml)
- https://github.com/ollama/ollama/blob/main/docs/api.md (Priority 4, docs/documentation_tech.yml)
