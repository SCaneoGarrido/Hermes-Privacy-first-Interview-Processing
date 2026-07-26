---
title: vcpkg - Documentacion Oficial
aliases: ["vcpkg"]
tags: [reference, vcpkg, buildsystem]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://learn.microsoft.com/vcpkg/
related: ["Stack Tecnologico de Hermes", "CMake - Documentacion Oficial", "Sprint 0 - Foundation"]
---

# Summary

Documentación oficial de vcpkg, el gestor de paquetes C/C++ usado por Hermes para resolver dependencias.

# Explanation

Fuentes oficiales:
- https://learn.microsoft.com/vcpkg/ — verificado 2026-07-24: "vcpkg is a cross-platform C/C++ package manager. Get access to thousands of high quality open-source libraries."
- https://github.com/microsoft/vcpkg — repositorio oficial.

La documentación cubre: instalación e integración con CMake (manifest mode), uso de vcpkg en Visual Studio y MSBuild, registries externos, versionado de dependencias, y caching binario/de assets para CI.

# Why it matters

Es la pieza que permite fijar versiones reproducibles de Crow, SQLite, whisper.cpp (dependencias), nlohmann/json, spdlog y Catch2 en cualquier máquina, sin instalación manual — soporta directamente el objetivo de "instalar Hermes en menos de 10 minutos" (ver [[Hermes - Vision General]]).

# Best Practices

Usar el modo manifest (`vcpkg.json`) en la raíz del repo en vez de instalar paquetes globalmente, para que el build sea reproducible por cualquier colaborador.

# Common Mistakes

Instalar dependencias con `vcpkg install <paquete>` de forma global e imperativa en vez de declararlas en `vcpkg.json`, rompiendo la reproducibilidad entre máquinas.

# Hermes Usage

Entregable explícito de [[Sprint 0 - Foundation]] ("Configuración vcpkg"); tema pendiente en el Knowledge Backlog para una nota propia (no listado aún en `.ai/KNOWLEDGE_BACKLOG.md`, sugerido añadirlo).

# Related Notes

- [[Stack Tecnologico de Hermes]]
- [[CMake - Documentacion Oficial]]
- [[Sprint 0 - Foundation]]

# References

- https://learn.microsoft.com/vcpkg/ (Priority 2, docs/documentation_tech.yml)
- https://github.com/microsoft/vcpkg (Priority 4, docs/documentation_tech.yml)
