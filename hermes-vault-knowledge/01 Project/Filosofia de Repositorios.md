---
title: Filosofia de Repositorios
aliases: ["Repository Philosophy"]
tags: [project, hermes, architecture]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Clean Architecture", "Repository Pattern", "Dependency Injection"]
---

# Summary

Toda dependencia externa en Hermes debe quedar aislada detrás de una interfaz.

# Explanation

Hermes exige que ninguna dependencia de infraestructura (base de datos, motor de transcripción, cliente LLM) sea usada directamente por la capa de dominio o aplicación. En su lugar, se define una interfaz abstracta y una implementación concreta:

- `ITranscriber` → `WhisperTranscriber`
- `ILLMClient` → `OllamaClient`

Los repositorios ocultan el acceso a SQLite; los controllers nunca contienen lógica de negocio (.ai/AI_INSTRUCTIONS.md: "Never access SQLite directly from controllers").

# Why it matters

Este aislamiento permite sustituir whisper.cpp, Ollama o SQLite por otra implementación sin afectar la lógica de dominio, y facilita el testing mediante dobles de prueba (mocks/fakes) de las interfaces.

# Best Practices

- Nombrar las interfaces con el prefijo `I` (ver [[Hermes Coding Standard]]).
- Inyectar las implementaciones concretas en tiempo de composición, nunca instanciarlas dentro del dominio.

# Common Mistakes

- Que un `Controller` llame directamente a SQLite o a la librería de whisper.cpp, saltándose la capa de repositorio/interfaz.

# Hermes Usage

Este es el fundamento práctico del [[Repository Pattern]] y de la [[Dependency Injection]] en Hermes, y es lo que hace cumplible la regla de [[Clean Architecture]]: "Frameworks are implementation details."

# Related Notes

- [[Clean Architecture]]
- [[Repository Pattern]]
- [[Dependency Injection]]

# References

- .ai/PROJECT.md (Priority 1)
- .ai/AI_INSTRUCTIONS.md (Priority 1)
