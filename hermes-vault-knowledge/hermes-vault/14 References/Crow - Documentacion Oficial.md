---
title: Crow - Documentacion Oficial
aliases: []
tags: [reference, crow, frameworks]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://crowcpp.org/master/
related: ["ADR-003 - Crow como Framework REST", "API First"]
---

# Summary

Documentación oficial y repositorio de Crow, el framework REST de Hermes.

# Explanation

Fuentes oficiales:
- https://crowcpp.org/master/ (verificada 2026-07-24: "fast and easy to use microframework for the web" para servicios HTTP y WebSocket en C++, inspirado en Flask)
- https://github.com/crowcpp/crow
- https://crowcpp.org/master/guides/ (listada en `docs/documentation_tech.yml`; al verificar el 2026-07-24 esta ruta específica devolvió 404 — el contenido de guías puede haberse reorganizado. Usar la raíz `https://crowcpp.org/master/` como punto de entrada confiable hasta reconfirmar la ruta.)
- https://crowcpp.org/master/reference/ (listada en `docs/documentation_tech.yml`, pendiente de verificar)

Crow es un micro-framework web para C++ inspirado en Flask/Express, elegido en [[ADR-003 - Crow como Framework REST]]. Las rutas se definen con la macro `CROW_ROUTE(app, "/ruta")` seguida de un lambda con la lógica del handler.

# Why it matters

Toda la superficie de la API REST de Hermes (ver [[API First]]) se construye con este framework; su documentación es la referencia de Priority 2 antes de recurrir a foros o blogs.

# Best Practices

Preferir la documentación oficial y el repositorio de GitHub sobre blogs no oficiales (.ai/OFFICIAL_SOURCES.md, instrucciones de investigación en Internet).

# Common Mistakes

N/A — nota de referencia.

# Hermes Usage

Ver pendiente: nota atómica "Crow Routing" en [[06 Frameworks]] (Knowledge Backlog, alta prioridad).

# Related Notes

- [[ADR-003 - Crow como Framework REST]]
- [[API First]]

# References

- https://crowcpp.org/master/ (Priority 2)
- https://github.com/crowcpp/crow (Priority 4)
- docs/documentation_tech.yml (Priority 2, categoría `crow`)
