---
title: nlohmann/json - Documentacion Oficial
aliases: ["nlohmann/json", "nlohmann json", "nlohmann/json - Documentacion Oficial"]
tags: [reference, json, cpp, libraries]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://json.nlohmann.me/
related: ["Stack Tecnologico de Hermes", "07 Libraries", "JSON - Especificacion Oficial"]
---

# Summary

Documentación oficial de nlohmann/json, la librería de serialización JSON de Hermes.

# Explanation

Fuentes oficiales:
- https://json.nlohmann.me/
- https://github.com/nlohmann/json

Verificado 2026-07-24: es una librería header-only de C++ moderno para manejo de JSON, que permite convertir directamente entre objetos C++ y JSON (parsear JSON a estructuras C++ y serializar estructuras C++ a JSON), sin configuración de build compleja.

# Why it matters

Es la librería que materializa los DTOs (ver [[DTO]]) de la REST API de Hermes: convierte las estructuras de `Application`/`REST API` a JSON y viceversa en los endpoints (Interview, Configuration, Health).

# Best Practices

Definir funciones `to_json`/`from_json` explícitas por DTO en vez de serializar entidades de dominio directamente (ver [[Filosofia de Repositorios]] y [[Clean Architecture]]).

# Common Mistakes

Usar `nlohmann::json` como tipo de retorno o parámetro en la capa `Domain`, filtrando una dependencia de infraestructura/serialización hacia el dominio.

# Hermes Usage

Tema pendiente de nota atómica propia en [[07 Libraries]] (no listado aún en `.ai/KNOWLEDGE_BACKLOG.md`).

# Related Notes

- [[Stack Tecnologico de Hermes]]
- [[07 Libraries]]
- [[JSON - Especificacion Oficial]]

# References

- https://json.nlohmann.me/ (Priority 2, docs/documentation_tech.yml)
- https://github.com/nlohmann/json (Priority 4, docs/documentation_tech.yml)
