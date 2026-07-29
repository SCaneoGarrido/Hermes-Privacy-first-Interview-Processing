---
title: Stack Tecnologico de Hermes
aliases: ["Hermes Tech Stack"]
tags: [project, hermes, stack]
status: stable
created: 2026-07-24
updated: 2026-07-29
source: .ai/PROJECT.md
related: ["Hermes - Vision General", "ADR-002 - C++20 como Lenguaje", "ADR-003 - Crow como Framework REST", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-005 - Ollama como Motor LLM", "ADR-008 - MySQL como Base de Datos", "ADR-009 - libmariadb como Cliente MySQL", "ADR-010 - Docker Compose para MySQL Local", "ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama"]
---

# Summary

Conjunto de tecnologías oficiales adoptadas por Hermes para backend, frontend, persistencia, IA y calidad.

# Explanation

| Área | Tecnología |
|---|---|
| Lenguaje | C++20 |
| Frontend | React + TypeScript + Vite |
| REST Framework | [[Crow - Documentacion Oficial|Crow]] |
| Base de datos | MySQL, corriendo en Docker Compose |
| Cliente de BD (C++) | [[MariaDB Connector-C (libmariadb)|libmariadb]] |
| Reconocimiento de voz | [[whisper.cpp - Repositorio Oficial|whisper.cpp]] |
| Normalizacion de audio | FFmpeg (avcodec/avformat/swresample, estatico via vcpkg — ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]) |
| LLM | [[Ollama - Documentacion Oficial|Ollama]] (modelo por defecto: Qwen, en uso: `qwen2.5:7b`) |
| Cliente HTTP (C++) | cpp-httplib (header-only, vía vcpkg — ver [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]) |
| Logging | spdlog |
| Build System | CMake |
| Gestor de paquetes | vcpkg |
| Testing | Catch2 |
| Serialización | nlohmann/json |

# Why it matters

Fijar el stack tecnológico evita la dispersión de dependencias y asegura que todas las decisiones de arquitectura sean coherentes con los principios [[Local First]] y [[Privacy First]]: cada tecnología elegida es ejecutable localmente y no requiere servicios externos.

# Best Practices

- No introducir una tecnología fuera de esta lista sin una nueva ADR que lo justifique (ver .ai/DECISIONS.md).
- Preferir la librería estándar de C++ sobre dependencias de terceros cuando sea razonable (.ai/AI_INSTRUCTIONS.md).

# Common Mistakes

- Añadir un framework de frontend adicional o un ORM sin evaluar su necesidad real.

# Hermes Usage

Cada entrada de esta tabla corresponde a una ADR específica en [[03 ADR]] que documenta el razonamiento detrás de la elección.

# Related Notes

- [[Hermes - Vision General]]
- [[ADR-002 - C++20 como Lenguaje]]
- [[ADR-003 - Crow como Framework REST]]
- [[ADR-004 - whisper.cpp para Reconocimiento de Voz]]
- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-008 - MySQL como Base de Datos]]
- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[ADR-010 - Docker Compose para MySQL Local]]
- [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]

# References

- .ai/PROJECT.md (Priority 1)
