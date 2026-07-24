---
title: Alcance y Exclusiones
aliases: ["Out of Scope", "Fuera de Alcance"]
tags: [project, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["Hermes - Vision General", "Privacy First", "ADR-007 - Prohibicion de Cloud por Defecto"]
---

# Summary

Lista de funcionalidades y arquitecturas que Hermes excluye explícitamente salvo aprobación explícita del usuario.

# Explanation

Fuera de alcance por defecto (.ai/PROJECT.md):

- Autenticación
- Procesamiento en la nube (Azure, OpenAI API, AWS, Google Cloud)
- Microservicios
- Kubernetes
- Procesamiento distribuido
- Message brokers
- Infraestructura compleja

Adicionalmente, para la versión 1.0 (README.md), quedan fuera: usuarios, roles, multiempresa, Docker obligatorio, PostgreSQL, sincronización online.

# Why it matters

Definir explícitamente qué NO se construye evita el scope creep y protege los principios [[Privacy First]] y Modular Monolith. Estas exclusiones son revaluables en versiones futuras, pero requieren aprobación explícita, no una decisión implícita durante el desarrollo.

# Best Practices

- Ante una feature request que roce esta lista, señalar el conflicto y pedir decisión explícita del usuario en vez de implementarla directamente.

# Common Mistakes

- Justificar una integración cloud como "solo para desarrollo" y dejarla filtrarse a producción.

# Hermes Usage

Esta nota debe consultarse antes de aceptar cualquier nueva dependencia o feature que implique red, autenticación multiusuario o infraestructura distribuida.

# Related Notes

- [[Hermes - Vision General]]
- [[Privacy First]]
- [[ADR-007 - Prohibicion de Cloud por Defecto]]

# References

- .ai/PROJECT.md (Priority 1)
- README.md (Priority 2, complementario)
