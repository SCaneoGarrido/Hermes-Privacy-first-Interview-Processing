---
title: ADR-015 - cpp-httplib como Cliente HTTP para Ollama
aliases: ["OllamaClient HTTP", "Cliente HTTP de Hermes"]
tags: [adr, hermes, libraries, ai, proposed]
status: proposed
created: 2026-07-28
updated: 2026-07-28
source: Diseño de sesión (Claude Code, pendiente de aprobación humana)
related: ["ADR-005 - Ollama como Motor LLM", "ADR-009 - libmariadb como Cliente MySQL", "ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio", "Ollama Integration Strategy", "Filosofia de Repositorios"]
---

# Context

`OllamaClient` (implementación de `ILLMClient`, ver [[Filosofia de Repositorios]] y [[Ollama Integration Strategy]]) necesita hacer requests HTTP salientes hacia la API REST local de Ollama (`POST /api/chat`, por defecto `http://localhost:11434`). Crow (el framework REST de Hermes, ADR-003) es solo servidor — no provee cliente HTTP saliente. Hace falta una librería nueva.

El tráfico es exclusivamente `localhost`, sin TLS, con requests/respuestas JSON simples (bloqueantes, ya que `OllamaClient` corre dentro del worker thread síncrono de Sprint 4).

# Decision

Usar **cpp-httplib** vía vcpkg, con `"default-features": false` (sin brotli/openssl/zlib/zstd — ninguno hace falta para hablarle a Ollama en localhost plano).

# Alternatives

- **libcurl**: la opción más madura y usada, pero su API en C es más verbosa para un caso de uso simple, y arrastra la posibilidad de necesitar OpenSSL según cómo la resuelva vcpkg — complejidad de build innecesaria para tráfico exclusivamente local sin TLS. Mismo criterio que ya se aplicó en [[ADR-009 - libmariadb como Cliente MySQL]]: preferir la opción de build más simple cuando cubre el caso de uso real.
- **Boost.Beast**: ya hay `boost-uuid` en el stack, pero Beast es una librería asio-based pensada para clientes/servidores asíncronos complejos — mucho más verbosa que lo que necesita un POST bloqueante simple.

# Consequences

- Nueva entrada en `vcpkg.json`: `cpp-httplib` con `default-features: false`. Header-only — no agrega tiempo de build relevante (a diferencia de FFmpeg/whisper.cpp en Sprint 5).
- `OllamaClient` hace requests bloqueantes (`httplib::Client::Post(...)`), consistente con que el worker de Sprint 4 procesa un job a la vez de forma síncrona - no hace falta manejo async.
- Si en el futuro Hermes necesita hablar con otro servicio HTTP por HTTPS, esta decisión hay que revisarla (agregar el feature `openssl` de cpp-httplib, o reconsiderar libcurl).

# Status

Proposed — pendiente de aprobación humana antes de agregarse a `vcpkg.json` e implementarse. Ver [[Ollama Integration Strategy]] y [[Sprint 6 - Ollama Integration]].

# References

- Sesión de diseño 2026-07-28 (Priority 1, este documento)
- [[ADR-009 - libmariadb como Cliente MySQL]] (precedente del mismo criterio de decisión)
- https://github.com/yhirose/cpp-httplib (Priority 4)
