---
title: API First
aliases: []
tags: [project, principle, hermes, architecture]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Hermes - Vision General", "Arquitectura en Capas de Hermes", "ADR-003 - Crow como Framework REST"]
---

# Summary

Principio inmutable de Hermes: toda la lógica de negocio se expone a través de una REST API; el frontend es únicamente un cliente de esa API.

# Explanation

El backend en C++20 expone toda su funcionalidad mediante endpoints REST construidos con [[Crow - Documentacion Oficial|Crow]]. El frontend en React no contiene lógica de negocio: consume la API igual que lo haría cualquier otro cliente (CLI, script, integración de terceros).

# Why it matters

Desacoplar el frontend del backend permite reemplazar o extender la interfaz (web, CLI, integración futura) sin tocar la lógica de dominio, y mantiene la arquitectura verificable: si una regla de negocio no es alcanzable vía API, no debería existir fuera de ella.

# Best Practices

- Ningún cálculo o regla de negocio debe vivir en el frontend.
- Todo endpoint nuevo debe versionarse (ver Sprint 1 — Core API, "Versionado API").

# Common Mistakes

- Implementar validaciones de negocio solo en el cliente React, dejando el backend sin esa garantía.

# Hermes Usage

Ver [[Arquitectura en Capas de Hermes]] para el flujo completo React → REST API → Application → Domain → Infrastructure.

# Related Notes

- [[Hermes - Vision General]]
- [[Arquitectura en Capas de Hermes]]
- [[ADR-003 - Crow como Framework REST]]

# References

- .ai/PROJECT.md (Priority 1)
