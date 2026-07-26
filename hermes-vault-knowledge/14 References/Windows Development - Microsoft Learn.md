---
title: Windows Development - Microsoft Learn
aliases: ["Windows Development"]
tags: [reference, windows, deployment]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://learn.microsoft.com/windows/
related: ["Sprint 12 - Release", "12 Deployment", "MSVC y C++ en Microsoft Learn"]
---

# Summary

Portal oficial de documentación de desarrollo para Windows, relevante para el instalador de Hermes.

# Explanation

Fuente: https://learn.microsoft.com/windows/

Verificado 2026-07-24: "Information for Windows application developers, hardware developers, and IT pros." Organiza el contenido en desarrollo de aplicaciones, herramientas de desarrollador, desarrollo de hardware/drivers, gestión para IT pros, Windows Server y Windows para IoT.

# Why it matters

Es la fuente base para el "Instalador Windows", entregable explícito de [[Sprint 12 - Release]] y de la meta de largo plazo "instalar Hermes en menos de 10 minutos" (ver [[Hermes - Vision General]]).

# Best Practices

Investigar opciones de empaquetado estándar de Windows (MSIX, instaladores MSI) antes de construir un instalador custom desde cero.

# Common Mistakes

Diseñar el instalador sin considerar permisos de usuario estándar (no-admin) en Windows, dificultando la instalación para investigadores sin privilegios elevados.

# Hermes Usage

Fuente de apoyo para [[12 Deployment]] (sección aún pendiente de desarrollo).

# Related Notes

- [[Sprint 12 - Release]]
- [[12 Deployment]]
- [[MSVC y C++ en Microsoft Learn]]

# References

- https://learn.microsoft.com/windows/ (Priority 2, docs/documentation_tech.yml)
