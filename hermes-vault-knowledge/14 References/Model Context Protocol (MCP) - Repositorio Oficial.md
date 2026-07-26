---
title: Model Context Protocol (MCP) - Repositorio Oficial
aliases: ["MCP", "Model Context Protocol", "Obsidian MCP"]
tags: [reference, mcp, tooling, no-adoptado]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://github.com/modelcontextprotocol
related: ["Obsidian - Ayuda Oficial"]
---

# Summary

Organización oficial del Model Context Protocol (MCP): **no está adoptado por Hermes actualmente**, listado en `docs/documentation_tech.yml` bajo la categoría `obsidian_mcp` como fuente disponible para una futura integración entre esta bóveda y asistentes de IA vía MCP.

# Explanation

Fuente: https://github.com/modelcontextprotocol

Verificado 2026-07-24: MCP es una iniciativa open-source que estandariza la conexión entre aplicaciones de modelos de lenguaje y fuentes de datos/herramientas externas ("an open protocol that enables seamless integration between LLM applications and external data sources and tools"). El proyecto ofrece SDKs en TypeScript, Python, Go y Rust, alojado por The Linux Foundation.

# Why it matters

Un servidor MCP para Obsidian permitiría que un agente de IA consulte esta bóveda (`hermes-vault-knowledge`) de forma estructurada en otras herramientas, más allá del acceso a archivos plano usado actualmente. Es una posibilidad futura, no una necesidad actual del proyecto Hermes.

# Best Practices

Si se evalúa un servidor MCP para esta bóveda, documentar la decisión explícitamente (no es una dependencia del backend de Hermes, sino de la tooling de conocimiento).

# Common Mistakes

Confundir esta entrada con una dependencia del backend de Hermes: es exclusivamente sobre tooling de la bóveda de conocimiento, no del producto.

# Hermes Usage

Ninguno actualmente; nota mantenida por completitud de `docs/documentation_tech.yml`.

# Related Notes

- [[Obsidian - Ayuda Oficial]]

# References

- https://github.com/modelcontextprotocol (Priority 4, docs/documentation_tech.yml)
