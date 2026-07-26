---
title: 06 Frameworks - Index
aliases: ["Frameworks Index", "06 Frameworks"]
tags: [moc, frameworks, index]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: .ai/KNOWLEDGE_BACKLOG.md
related: ["Crow - Documentacion Oficial", "CMake - Documentacion Oficial"]
---

# Summary

Índice de notas atómicas sobre los frameworks usados por Hermes. Sección pendiente de desarrollo.

# Explanation

Temas identificados en `.ai/KNOWLEDGE_BACKLOG.md` (alta prioridad) aún sin nota atómica dedicada:

- [ ] Crow Routing
- [ ] CMake Presets

Fuentes previstas: [[Crow - Documentacion Oficial]], [[CMake - Documentacion Oficial]]. El gestor de paquetes [[vcpkg - Documentacion Oficial|vcpkg]], aunque no es un framework, es la pieza que integra estas dependencias con CMake (ver [[Sprint 0 - Foundation]]).

# Why it matters

"Crow Routing" documentará cómo se implementan los endpoints REST de Hermes ([[API First]]); "CMake Presets" estandarizará la configuración de build entre entornos (Windows/Linux, Debug/Release).

# Best Practices

Documentar el enrutamiento de Crow con ejemplos concretos de los endpoints reales de Hermes (Interview, Configuration, Health), no ejemplos genéricos.

# Common Mistakes

Confundir esta sección con [[07 Libraries]]: aquí van los frameworks estructurales (Crow, CMake); las librerías puntuales (SQLite, nlohmann/json, spdlog) van en Libraries.

# Hermes Usage

Referenciado por [[Sprint 0 - Foundation]] y [[Sprint 1 - Core API]].

# Related Notes

- [[Crow - Documentacion Oficial]]
- [[CMake - Documentacion Oficial]]
- [[vcpkg - Documentacion Oficial]]

# References

- .ai/KNOWLEDGE_BACKLOG.md (Priority 1)
- docs/documentation_tech.yml (Priority 2, categorías `crow`, `cmake`, `vcpkg`)
