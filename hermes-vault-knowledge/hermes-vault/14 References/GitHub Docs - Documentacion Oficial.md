---
title: GitHub Docs - Documentacion Oficial
aliases: ["GitHub Docs", "GitHub"]
tags: [reference, github, tooling]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://docs.github.com/
related: ["Sprint 12 - Release", "Sprint 11 - Documentation", "Git - Documentacion Oficial"]
---

# Summary

Documentación oficial de GitHub, la plataforma donde Hermes se aloja como repositorio público.

# Explanation

Fuente oficial: https://docs.github.com/

Verificado 2026-07-24: cubre repositorios (almacenar y colaborar en código), automatización (GitHub Actions), colaboración (pull requests, issues), herramientas de desarrollador (Copilot, Codespaces, CLI), gestión enterprise, seguridad, APIs, y hosting estático (GitHub Pages).

# Why it matters

El "repositorio público" es un entregable explícito de [[Sprint 12 - Release]]; GitHub Actions es la opción más directa para automatizar build/test de Hermes en CI sin infraestructura propia (coherente con evitar "Complex infrastructure", ver [[Alcance y Exclusiones]]).

# Best Practices

Usar GitHub Actions para validar que cada PR compila y pasa los tests de [[Sprint 10 - Testing]] antes de mergear a `main`/`develop`.

# Common Mistakes

Documentar el proceso de release ([[Sprint 12 - Release]]) sin usar GitHub Releases para versionar los binarios del instalador de Windows.

# Hermes Usage

Plataforma de alojamiento y CI/CD potencial para Hermes; también referenciada por el "Contribution Guide" de [[Sprint 11 - Documentation]].

# Related Notes

- [[Sprint 12 - Release]]
- [[Sprint 11 - Documentation]]
- [[Git - Documentacion Oficial]]

# References

- https://docs.github.com/ (Priority 2, docs/documentation_tech.yml)
