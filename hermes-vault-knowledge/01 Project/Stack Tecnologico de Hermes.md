---
title: Stack Tecnologico de Hermes
aliases: ["Hermes Tech Stack"]
tags: [project, hermes, stack]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Hermes - Vision General", "ADR-002 - C++20 como Lenguaje", "ADR-003 - Crow como Framework REST", "ADR-004 - whisper.cpp para Reconocimiento de Voz", "ADR-005 - Ollama como Motor LLM", "ADR-006 - SQLite como Base de Datos"]
---

# Summary

Conjunto de tecnologías oficiales adoptadas por Hermes para backend, frontend, persistencia, IA y calidad.

# Explanation

| Área | Tecnología |
|---|---|
| Lenguaje | C++20 |
| Frontend | React + TypeScript |
| REST Framework | [[Crow - Documentacion Oficial|Crow]] |
| Base de datos | SQLite |
| Reconocimiento de voz | [[whisper.cpp - Repositorio Oficial|whisper.cpp]] |
| LLM | [[Ollama - Documentacion Oficial|Ollama]] (modelo por defecto: Qwen) |
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
- [[ADR-006 - SQLite como Base de Datos]]

# References

- .ai/PROJECT.md (Priority 1)
