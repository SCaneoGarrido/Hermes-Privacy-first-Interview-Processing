---
title: doctest - Repositorio Oficial
aliases: ["doctest"]
tags: [reference, doctest, testing, no-adoptado]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/doctest/doctest
related: ["Catch2 - Repositorio Oficial", "Sprint 10 - Testing"]
---

# Summary

Framework de testing en C++ **no adoptado por Hermes** (el proyecto usa Catch2), incluido en `docs/documentation_tech.yml` como fuente oficial disponible para referencia/comparación.

# Explanation

Fuente oficial: https://github.com/doctest/doctest

Verificado 2026-07-24: doctest es un framework de testing en C++ de un solo header, diseñado para permitir escribir tests directamente en el código de producción. Según su propia documentación, es "por lejos el más rápido tanto en tiempos de compilación (por órdenes de magnitud) como en runtime" comparado con Catch2, manteniendo funcionalidad similar.

# Why it matters

Es relevante solo como punto de comparación: si en el futuro los tiempos de compilación de los tests con Catch2 se vuelven un problema (ver [[Sprint 10 - Testing]]), doctest sería la alternativa a evaluar — pero cambiar de framework de testing requeriría una nueva ADR, no una decisión implícita.

# Best Practices

No mezclar doctest y Catch2 en el mismo proyecto sin una decisión explícita documentada.

# Common Mistakes

Asumir que Hermes usa doctest porque aparece en `docs/documentation_tech.yml`: el stack oficial (.ai/PROJECT.md) especifica Catch2.

# Hermes Usage

Ninguno actualmente. Nota mantenida solo por completitud de fuentes oficiales del proyecto.

# Related Notes

- [[Catch2 - Repositorio Oficial]]
- [[Sprint 10 - Testing]]

# References

- https://github.com/doctest/doctest (Priority 4, docs/documentation_tech.yml)
