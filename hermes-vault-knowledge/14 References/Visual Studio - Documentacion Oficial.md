---
title: Visual Studio - Documentacion Oficial
aliases: ["Visual Studio"]
tags: [reference, visual-studio, tooling]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://learn.microsoft.com/visualstudio/
related: ["MSVC y C++ en Microsoft Learn", "Visual Studio Code - Documentacion Oficial"]
---

# Summary

Documentación oficial del IDE Visual Studio, distinta de la documentación del compilador MSVC.

# Explanation

Fuente: https://learn.microsoft.com/visualstudio/

Verificado 2026-07-24: cubre la familia de productos Visual Studio (IDE para Windows/Mac), Visual Studio Code, guías de lenguaje (C/C++, C#, Python, JavaScript/TypeScript, F#), GitHub Copilot integrado, y Microsoft Dev Box.

# Why it matters

Es el IDE más directo para desarrollar Hermes en Windows con integración nativa de CMake y depuración de C++; distinto de [[MSVC y C++ en Microsoft Learn]], que documenta el compilador/librería estándar en sí.

# Best Practices

Usar la integración nativa de CMake de Visual Studio (`CMakeLists.txt` abierto directamente, sin generar un proyecto `.sln` intermedio) para no duplicar configuración de build.

# Common Mistakes

Mantener configuración de build específica de Visual Studio (`.sln`, `.vcxproj`) en paralelo a `CMakeLists.txt`, generando dos fuentes de verdad para el build.

# Hermes Usage

IDE recomendado para desarrollo en Windows; no introduce una dependencia de build (el proyecto sigue siendo CMake + vcpkg, ver [[CMake - Documentacion Oficial]]).

# Related Notes

- [[MSVC y C++ en Microsoft Learn]]
- [[Visual Studio Code - Documentacion Oficial]]

# References

- https://learn.microsoft.com/visualstudio/ (Priority 2, docs/documentation_tech.yml)
