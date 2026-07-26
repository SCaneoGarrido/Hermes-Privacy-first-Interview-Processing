---
title: ADR
aliases: ["Architecture Decision Record"]
tags: [glossary, process]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/DECISIONS.md
related: ["03 ADR"]
---

# Summary

Registro formal de una decisión de arquitectura: su contexto, la decisión tomada, alternativas consideradas y consecuencias.

# Explanation

En Hermes, cada ADR sigue la estructura: Context, Decision, Alternatives, Consequences, Status, References. Las ADRs viven en [[03 ADR]] y nunca se eliminan del historial, aunque queden obsoletas (se marcan con un nuevo `status`).

# Why it matters

Preserva el razonamiento detrás de decisiones técnicas (por ejemplo, [[ADR-006 - SQLite como Base de Datos]]) para que futuros colaboradores no las reviertan sin conocer el contexto original.

# Best Practices

Crear una ADR nueva cada vez que el usuario tome una decisión arquitectónica, nunca sobrescribir una ADR existente.

# Common Mistakes

Documentar una decisión técnica menor (por ejemplo, el nombre de una variable) como ADR: las ADR son para decisiones de arquitectura, no de implementación de detalle.

# Hermes Usage

Ver el índice completo en [[03 ADR]].

# Related Notes

- [[03 ADR]]

# References

- .ai/DECISIONS.md (Priority 1)
