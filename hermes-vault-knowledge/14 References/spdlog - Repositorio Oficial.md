---
title: spdlog - Repositorio Oficial
aliases: ["spdlog"]
tags: [reference, spdlog, logging]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/gabime/spdlog
related: ["Stack Tecnologico de Hermes", "11 Development"]
---

# Summary

Repositorio oficial de spdlog, la librería de logging de Hermes.

# Explanation

Fuentes oficiales:
- https://github.com/gabime/spdlog
- https://github.com/gabime/spdlog/wiki

Verificado 2026-07-24: spdlog es una librería de logging de alto rendimiento para C++, disponible tanto header-only como compilada. Soporta múltiples "sinks" (archivo con rotación, logs diarios, consola con colores, syslog), formato y niveles de log configurables, y logging asíncrono para escenarios de alto throughput. Uso básico: `spdlog::info("mensaje");`.

# Why it matters

Es el logging usado en todos los sprints de Hermes (mencionado como entregable en [[Sprint 0 - Foundation]] y [[Sprint 1 - Core API]]); su modo asíncrono es relevante para no bloquear los worker threads de [[Sprint 4 - Background Processing]].

# Best Practices

Configurar niveles de log distintos por sink (por ejemplo, `debug` a archivo, `warn` a consola) en vez de un único nivel global.

# Common Mistakes

Loguear de forma síncrona dentro de rutas críticas de rendimiento (por ejemplo, dentro del loop de transcripción) sin usar el logger asíncrono.

# Hermes Usage

Tema pendiente de nota atómica propia en [[11 Development]] (no listado aún en `.ai/KNOWLEDGE_BACKLOG.md`, sugerido añadirlo con prioridad media).

# Related Notes

- [[Stack Tecnologico de Hermes]]
- [[11 Development]]

# References

- https://github.com/gabime/spdlog (Priority 4, docs/documentation_tech.yml)
- https://github.com/gabime/spdlog/wiki (Priority 4, docs/documentation_tech.yml)
