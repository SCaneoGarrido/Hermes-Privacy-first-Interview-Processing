---
title: MOC - Referencias
aliases: ["References MOC", "14 References"]
tags: [moc, references]
status: stable
created: 2026-07-24
updated: 2026-07-28
source: .ai/OFFICIAL_SOURCES.md
related: ["Home", "Politica de Fuentes Tecnicas"]
---

# Summary

Mapa de contenido de la sección [[14 References]]: fuentes oficiales y libros aprobados usados por Hermes.

# Explanation

Ver [[Politica de Fuentes Tecnicas]] para las reglas de priorización y las fuentes a evitar (`docs/documentation_tech.yml`).

**Backend / C++ / Build**
- [[C++ - Referencias del Lenguaje]] (cppreference, C++ Core Guidelines, isocpp, WG21)
- [[CMake - Documentacion Oficial]]
- [[vcpkg - Documentacion Oficial]]
- [[Crow - Documentacion Oficial]]
- [[MySQL - Documentacion Oficial]]
- [[MariaDB Connector-C - Documentacion Oficial]]
- [[Docker - Documentacion Oficial]]

**Logging, serialización y testing**
- [[spdlog - Repositorio Oficial]]
- [[fmt - Documentacion Oficial]]
- [[nlohmann/json - Documentacion Oficial]]
- [[Catch2 - Repositorio Oficial]]
- [[doctest - Repositorio Oficial]] (no adoptado, referencia comparativa)

**IA local**
- [[whisper.cpp - Repositorio Oficial]]
- [[ggml - Repositorio Oficial]]
- [[Ollama - Documentacion Oficial]]

**Frontend**
- [[React - Documentacion Oficial]]
- [[TypeScript - Documentacion Oficial]]
- [[Vite - Documentacion Oficial]]
- [[Node.js - Documentacion Oficial]]

**Diseño de API / Formatos**
- [[HTTP - Referencia MDN]]
- [[JSON - Especificacion Oficial]]
- [[REST API Design - Microsoft Learn]]

**Tooling y plataforma**
- [[Git - Documentacion Oficial]]
- [[GitHub Docs - Documentacion Oficial]]
- [[Markdown Guide - Referencia]]
- [[Unicode - Referencia Oficial]]
- [[Windows Development - Microsoft Learn]]
- [[MSVC y C++ en Microsoft Learn]]
- [[Visual Studio - Documentacion Oficial]]
- [[Visual Studio Code - Documentacion Oficial]]
- [[Obsidian - Ayuda Oficial]]
- [[Model Context Protocol (MCP) - Repositorio Oficial]] (no adoptado, referencia futura)

**No adoptadas por Hermes** (disponibles como fuente, sin ADR asociada)
- [[OpenSSL - Documentacion Oficial]]
- [[Boost - Documentacion Oficial]]
- [[doctest - Repositorio Oficial]]
- [[Model Context Protocol (MCP) - Repositorio Oficial]]
- [[SQLite - Documentacion Oficial]] (adoptada originalmente vía [[ADR-006 - SQLite como Base de Datos|ADR-006]], superada por [[ADR-008 - MySQL como Base de Datos|ADR-008]])

**Libros aprobados (Priority 3)**
- [[Libro - Clean Architecture (Robert C. Martin)]]
- [[Libro - Code Complete 2nd Edition (Steve McConnell)]]
- [[Libro - Professional C++ 5th Edition (Marc Gregoire)]]

# Why it matters

Centraliza el orden de autoridad de las fuentes (ver reglas de "Source of Truth" del curador) en un único mapa navegable.

# Best Practices

Antes de citar una fuente en cualquier nota nueva, verificar que ya tenga una nota de referencia aquí; si no, crearla.

# Common Mistakes

Citar un blog o Stack Overflow como si fuera una fuente oficial (prohibido salvo indicarlo explícitamente como opinión de comunidad).

# Hermes Usage

N/A — nota de organización.

# Related Notes

- [[Home]]
- [[Politica de Fuentes Tecnicas]]

# References

- .ai/OFFICIAL_SOURCES.md (Priority 1)
- docs/documentation_tech.yml (Priority 2 — fuente principal de la actualización del 2026-07-24)
