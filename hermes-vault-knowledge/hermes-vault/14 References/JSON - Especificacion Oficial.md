---
title: JSON - Especificacion Oficial
aliases: ["JSON"]
tags: [reference, json, format]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://www.json.org/json-en.html
related: ["nlohmann/json - Documentacion Oficial", "API First"]
---

# Summary

Especificación oficial del formato JSON, usado en toda la REST API de Hermes.

# Explanation

Fuente: https://www.json.org/json-en.html

Verificado 2026-07-24: JSON es "un formato de intercambio de datos ligero", fácil de leer/escribir para humanos y fácil de parsear/generar para máquinas. Se basa en dos estructuras universales: objetos (pares nombre/valor) y arrays (colecciones ordenadas de valores).

# Why it matters

Es el formato de serialización de toda la comunicación entre React y la REST API de Hermes (ver [[API First]]), implementado en C++ mediante [[nlohmann/json - Documentacion Oficial|nlohmann/json]].

# Best Practices

Mantener los DTOs (ver [[DTO]]) como JSON planos y explícitos, evitando estructuras anidadas innecesarias que compliquen el consumo desde TypeScript.

# Common Mistakes

Serializar tipos no representables nativamente en JSON (fechas, `NaN`, `Infinity`) sin una convención explícita de codificación (por ejemplo, ISO 8601 para fechas).

# Hermes Usage

Formato de intercambio de todos los endpoints REST de Hermes.

# Related Notes

- [[nlohmann/json - Documentacion Oficial]]
- [[API First]]

# References

- https://www.json.org/json-en.html (Priority 2, docs/documentation_tech.yml)
