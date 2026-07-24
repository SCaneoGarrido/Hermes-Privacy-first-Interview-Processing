---
title: Sprint 3 - File Upload
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-24
source: README.md
related: ["Sprint 2 - Persistence", "Sprint 4 - Background Processing"]
---

# Summary

Carga de audios de entrevistas mediante upload multipart.

# Explanation

**Objetivo:** carga de audios.

**Entregables:** upload multipart, validación, organización de archivos, identificadores únicos, validaciones.

# Why it matters

Es el punto de entrada de datos sensibles al sistema; su validación (tamaño, formato, integridad) es la primera línea de defensa antes de que el audio llegue a whisper.cpp.

# Best Practices

- Validar tipo y tamaño de archivo en el backend, no solo en el cliente React ([[API First]]).

# Common Mistakes

- Confiar únicamente en la extensión del archivo para determinar su formato real.

# Hermes Usage

Prepara el terreno para [[Sprint 4 - Background Processing]] y [[Sprint 5 - Whisper Integration]].

# Related Notes

- [[Sprint 2 - Persistence]]
- [[Sprint 4 - Background Processing]]

# References

- README.md (Priority 2)
