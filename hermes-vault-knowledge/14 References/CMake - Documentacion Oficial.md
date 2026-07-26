---
title: CMake - Documentacion Oficial
aliases: []
tags: [reference, cmake, buildsystem]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://cmake.org/documentation/
related: ["Stack Tecnologico de Hermes"]
---

# Summary

Documentación oficial de CMake, el sistema de build usado por Hermes.

# Explanation

Fuentes oficiales:
- https://cmake.org/documentation/
- https://cmake.org/cmake/help/latest/ (referencia completa de comandos y variables)
- https://cmake.org/cmake/help/latest/guide/tutorial/ (tutorial oficial paso a paso, verificado 2026-07-24: 11 pasos desde un ejecutable básico hasta testing, instalación y gestión de dependencias; menciona `CMakePresets.json` en el paso 3 sin profundizar en el detalle)

CMake genera los archivos de build (Makefiles, Ninja, Visual Studio) a partir de los `CMakeLists.txt` del proyecto. Es la herramienta que orquesta la compilación de Hermes junto con [[vcpkg - Documentacion Oficial|vcpkg]] para la resolución de dependencias.

# Why it matters

Es el punto de entrada para cualquier persona que quiera compilar Hermes desde cero; su configuración correcta es un entregable explícito de [[Sprint 0 - Foundation]].

# Best Practices

Consultar siempre la documentación oficial antes que tutoriales de terceros (ver .ai/OFFICIAL_SOURCES.md).

# Common Mistakes

N/A — nota de referencia.

# Hermes Usage

Ver pendiente: nota atómica "CMake Presets" en [[06 Frameworks]] (Knowledge Backlog).

# Related Notes

- [[Stack Tecnologico de Hermes]]

# References

- https://cmake.org/documentation/ (Priority 2, .ai/OFFICIAL_SOURCES.md)
- https://cmake.org/cmake/help/latest/ (Priority 2, docs/documentation_tech.yml)
- https://cmake.org/cmake/help/latest/guide/tutorial/ (Priority 2, docs/documentation_tech.yml)
