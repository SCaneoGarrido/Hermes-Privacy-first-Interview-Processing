---
title: TypeScript - Documentacion Oficial
aliases: ["TypeScript"]
tags: [reference, typescript, frontend]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://www.typescriptlang.org/docs/
related: ["Sprint 8 - Frontend", "React - Documentacion Oficial", "Vite - Documentacion Oficial"]
---

# Summary

Documentación oficial de TypeScript, el lenguaje del frontend de Hermes (React + TypeScript).

# Explanation

Fuente oficial: https://www.typescriptlang.org/docs/

Verificado 2026-07-24: TypeScript es un superset de JavaScript con tipado estático. La documentación oficial organiza recursos por nivel de experiencia (nuevos programadores, desarrolladores JS, desarrolladores OOP) e incluye handbooks completos (tipos básicos, genéricos, tipos condicionales, archivos de declaración), tutoriales de integración con distintas herramientas de build, referencia de configuración (`tsconfig.json`) y cheat sheets.

# Why it matters

El tipado estático de TypeScript reduce errores de contrato entre el frontend y los DTOs expuestos por la REST API de Hermes (ver [[API First]]), detectando en tiempo de compilación discrepancias que en JavaScript puro solo aparecerían en runtime.

# Best Practices

Definir tipos TypeScript para los DTOs de la API que reflejen exactamente el contrato REST (ver [[DTO]]), idealmente generados o validados contra la documentación real de los endpoints.

# Common Mistakes

Usar `any` para evitar errores de tipado en las respuestas de la API, perdiendo la principal ventaja de TypeScript.

# Hermes Usage

Lenguaje del frontend construido en [[Sprint 8 - Frontend]], junto con [[React - Documentacion Oficial|React]] y [[Vite - Documentacion Oficial|Vite]].

# Related Notes

- [[Sprint 8 - Frontend]]
- [[React - Documentacion Oficial]]
- [[Vite - Documentacion Oficial]]

# References

- https://www.typescriptlang.org/docs/ (Priority 2, docs/documentation_tech.yml)
