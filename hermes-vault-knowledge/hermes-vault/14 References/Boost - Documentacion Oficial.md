---
title: Boost - Documentacion Oficial
aliases: ["Boost"]
tags: [reference, boost, cpp, no-adoptado]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://www.boost.org/doc/
related: ["05 C++", "Hermes Coding Standard"]
---

# Summary

Documentación oficial de Boost: **no es una dependencia actual de Hermes**, listada en `docs/documentation_tech.yml` como fuente disponible.

# Explanation

Fuente oficial: https://www.boost.org/libraries/ (la URL `boost.org/doc/` listada en el YAML redirige de forma permanente a esta, verificado 2026-07-24).

Boost es una colección de librerías C++ revisadas por pares que extienden la librería estándar, cubriendo algoritmos, contenedores, programación concurrente, procesamiento de strings, matemáticas y E/S de red de bajo nivel.

# Why it matters

Choca directamente con la regla de `.ai/AI_INSTRUCTIONS.md`: "Prefer standard library over third-party libraries" y "Do not introduce unnecessary dependencies". Boost debería considerarse solo si la librería estándar de C++20 y las dependencias ya adoptadas (Crow, SQLite, whisper.cpp, Ollama, nlohmann/json, spdlog, fmt, Catch2) no cubren una necesidad concreta.

# Best Practices

Antes de añadir cualquier módulo de Boost, verificar si C++20 (`std::filesystem`, `std::format`, coroutines, ranges) ya resuelve la necesidad.

# Common Mistakes

Añadir Boost completo por una sola utilidad (por ejemplo, `boost::asio`) cuando una librería más pequeña y específica bastaría.

# Hermes Usage

Ninguno actualmente.

# Related Notes

- [[05 C++]]
- [[Hermes Coding Standard]]

# References

- https://www.boost.org/libraries/ (Priority 2, docs/documentation_tech.yml — reemplaza a boost.org/doc/, redirect 301 verificado 2026-07-24)
