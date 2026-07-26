---
title: Catch2 - Repositorio Oficial
aliases: ["Catch2"]
tags: [reference, catch2, testing]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/catchorg/Catch2
related: ["Stack Tecnologico de Hermes", "Sprint 10 - Testing", "doctest - Repositorio Oficial"]
---

# Summary

Documentación oficial de Catch2, el framework de testing adoptado por Hermes.

# Explanation

Fuentes oficiales:
- https://github.com/catchorg/Catch2
- https://catch2-temp.readthedocs.io/en/latest/ (nombre de dominio temporal según lo lista `docs/documentation_tech.yml`; verificar vigencia antes de citarla como definitiva)

Verificado 2026-07-24: Catch2 es un framework moderno de testing en C++ (C++14/17+) para unit tests, TDD y BDD. Su sintaxis es natural: los nombres de test no requieren ser identificadores válidos, las aserciones se parecen a expresiones C++ normales, y las `SECTION` gestionan setup/teardown de forma elegante. Ofrece macros `TEST_CASE` y aserciones `REQUIRE`, además de micro-benchmarking. Desde la v3 pasó de ser una librería de un solo header a una librería multi-header convencional, aunque sigue sin dependencias externas.

# Why it matters

Es el framework elegido para todos los tests de Hermes (unitarios, de integración y de estrés) en [[Sprint 10 - Testing]].

# Best Practices

Usar `SECTION` para casos con setup compartido en vez de duplicar el `TEST_CASE` completo.

# Common Mistakes

Escribir tests de integración que dependan de whisper.cpp/Ollama reales sin marcarlos como tales (ver [[Sprint 10 - Testing]]) — Catch2 no distingue esto automáticamente, es responsabilidad del equipo organizarlo (por tags de Catch2, por ejemplo).

# Hermes Usage

Framework de testing elegido en el stack (.ai/PROJECT.md); ver también [[doctest - Repositorio Oficial]] como alternativa **no adoptada** por Hermes, incluida en `docs/documentation_tech.yml` solo como fuente de referencia disponible.

# Related Notes

- [[Stack Tecnologico de Hermes]]
- [[Sprint 10 - Testing]]
- [[doctest - Repositorio Oficial]]

# References

- https://github.com/catchorg/Catch2 (Priority 4, docs/documentation_tech.yml)
- https://catch2-temp.readthedocs.io/en/latest/ (Priority 2, docs/documentation_tech.yml — verificar vigencia del dominio)
