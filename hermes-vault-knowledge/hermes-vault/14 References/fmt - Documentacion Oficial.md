---
title: fmt - Documentacion Oficial
aliases: ["fmtlib", "{fmt}"]
tags: [reference, fmt, cpp]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://fmt.dev/latest/index.html
related: ["spdlog - Repositorio Oficial", "05 C++"]
---

# Summary

Documentación oficial de la librería fmt, usada internamente por spdlog para el formateo de mensajes de log.

# Explanation

Fuentes oficiales:
- https://fmt.dev/latest/index.html
- https://github.com/fmtlib/fmt

Verificado 2026-07-24: fmt es una utilidad de formateo moderna en C++ que actúa como reemplazo seguro de la familia `printf`, con verificación de errores en tiempo de compilación y gestión automática de memoria. Es la librería que inspiró `std::format` de C++20 (no confirmado explícitamente en la página consultada, pero es de conocimiento público en la comunidad C++ y coherente con la propia documentación de fmt).

# Why it matters

No es una dependencia directa declarada en `.ai/PROJECT.md`, pero **spdlog la usa internamente** para formatear mensajes de log; entenderla ayuda a diagnosticar errores de formato en los logs de Hermes.

# Best Practices

Si Hermes necesita formateo de strings fuera de logging, evaluar `std::format` (C++20, ya adoptado vía [[ADR-002 - C++20 como Lenguaje]]) antes de añadir fmt como dependencia directa adicional — evita duplicar funcionalidad ya cubierta por la librería estándar (.ai/AI_INSTRUCTIONS.md: "Prefer standard library over third-party libraries").

# Common Mistakes

Añadir fmt como dependencia explícita del proyecto sin verificar primero si `std::format` de C++20 ya cubre la necesidad.

# Hermes Usage

Dependencia transitiva vía spdlog; no tiene uso directo documentado en Hermes por ahora.

# Related Notes

- [[spdlog - Repositorio Oficial]]
- [[05 C++]]

# References

- https://fmt.dev/latest/index.html (Priority 2, docs/documentation_tech.yml)
- https://github.com/fmtlib/fmt (Priority 4, docs/documentation_tech.yml)
