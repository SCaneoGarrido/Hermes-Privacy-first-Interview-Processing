---
title: Git - Documentacion Oficial
aliases: ["Git"]
tags: [reference, git, tooling]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://git-scm.com/doc
related: ["Sprint 0 - Foundation", "Sprint 11 - Documentation"]
---

# Summary

Documentación oficial de Git, el sistema de control de versiones de Hermes.

# Explanation

Fuente oficial: https://git-scm.com/docs (la URL `git-scm.com/doc` redirige a esta, verificado 2026-07-24).

Organiza la referencia en: comandos de configuración inicial, snapshotting básico (`add`, `commit`, `reset`), branching y merging, sincronización (`fetch`, `pull`, `push`), parcheo, debugging, administración y "plumbing commands"; además de guías, tutoriales, FAQ y el Git Book oficial.

# Why it matters

El repositorio Git es el primer entregable de [[Sprint 0 - Foundation]]; las convenciones de branching de Hermes (`main`, `develop`, `feature/*`, `fix/*`, `release/*`, `hotfix/*`, ver README.md) dependen de un uso correcto de branching y merging documentado aquí.

# Best Practices

Seguir Conventional Commits o un estilo de mensajes consistente (no especificado aún en `.ai/`, sugerido documentarlo si el equipo adopta una convención).

# Common Mistakes

Usar `git push --force` sobre ramas compartidas sin coordinación — no específico de Hermes, pero relevante para cualquier colaborador nuevo del proyecto open-source.

# Hermes Usage

Herramienta base de todo el flujo de desarrollo; el "Contribution Guide" de [[Sprint 11 - Documentation]] debería enlazar esta referencia.

# Related Notes

- [[Sprint 0 - Foundation]]
- [[Sprint 11 - Documentation]]

# References

- https://git-scm.com/docs (Priority 2, docs/documentation_tech.yml)
