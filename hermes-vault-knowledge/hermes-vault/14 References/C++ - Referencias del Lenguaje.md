---
title: C++ - Referencias del Lenguaje
aliases: ["cppreference", "CppCoreGuidelines", "isocpp"]
tags: [reference, cpp, language]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://en.cppreference.com/
related: ["ADR-002 - C++20 como Lenguaje", "05 C++", "Hermes Coding Standard"]
---

# Summary

Conjunto de fuentes oficiales/de referencia del lenguaje C++ que Hermes usa para C++20: cppreference, C++ Core Guidelines, isocpp.org y los papers de WG21.

# Explanation

Fuentes (categoría `cpp` en `docs/documentation_tech.yml`):
- https://en.cppreference.com/ — referencia técnica del lenguaje y la librería estándar (no es un documento ISO oficial, pero es el estándar de facto de la comunidad C++ y ampliamente citado como fuente confiable).
- https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines — mantenidas por Bjarne Stroustrup y Herb Sutter; verificado 2026-07-24: cubren 15 áreas (filosofía, interfaces, funciones, clases, plantillas, manejo de errores, concurrencia, etc.) con reglas concretas y justificación. Enfatizan RAII, smart pointers para semántica de ownership, seguridad de tipos y de memoria — directamente alineado con la [[Hermes Coding Standard]].
- https://isocpp.org/ — portal de la Standard C++ Foundation.
- https://wg21.link/ — acceso directo a los papers del comité de estandarización (WG21), Priority 5 en la jerarquía de fuentes del curador.

# Why it matters

Es la base de conocimiento para desarrollar las notas pendientes de [[05 C++]] (RAII, Smart Pointers, Coroutines) con contenido verificado en vez de solo memoria del modelo.

# Best Practices

Al escribir una nota atómica de C++, citar cppreference para la mecánica exacta y las C++ Core Guidelines para la justificación de buenas prácticas — son complementarias, no intercambiables.

# Common Mistakes

Tratar cppreference como un estándar ISO oficial: es una referencia de la comunidad, de gran calidad pero no normativa (la norma es el estándar ISO, accesible vía WG21).

# Hermes Usage

Fuente primaria prevista para poblar [[05 C++]] (RAII, Smart Pointers) y para verificar que la [[Hermes Coding Standard]] es coherente con C++20 ([[ADR-002 - C++20 como Lenguaje]]).

# Related Notes

- [[ADR-002 - C++20 como Lenguaje]]
- [[05 C++]]
- [[Hermes Coding Standard]]

# References

- https://en.cppreference.com/ (Priority 2/5, docs/documentation_tech.yml)
- https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines (Priority 2, docs/documentation_tech.yml, verificado 2026-07-24)
- https://isocpp.org/ (Priority 2, docs/documentation_tech.yml)
- https://wg21.link/ (Priority 5, docs/documentation_tech.yml)
