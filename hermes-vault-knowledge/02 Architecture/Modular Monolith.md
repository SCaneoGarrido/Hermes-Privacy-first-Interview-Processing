---
title: Modular Monolith
aliases: ["Monolito Modular"]
tags: [architecture, pattern, hermes]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/PROJECT.md
related: ["ADR-001 - Modular Monolith", "Arquitectura en Capas de Hermes", "Alcance y Exclusiones"]
---

# Summary

Hermes es un único ejecutable compuesto por módulos independientes, explícitamente NO una arquitectura de microservicios.

# Explanation

"Modular Monolith" describe un sistema desplegado como un solo proceso/binario, pero organizado internamente en módulos con responsabilidades bien delimitadas y bajo acoplamiento entre sí. Esto contrasta con microservicios (procesos independientes comunicados por red) y con una arquitectura desestructurada tipo "big ball of mud".

# Why it matters

Para Hermes, un monolito modular es suficiente porque el objetivo es que un investigador instale y ejecute la aplicación en menos de 10 minutos, sin orquestar múltiples servicios. La modularidad interna preserva la mantenibilidad sin la complejidad operativa de microservicios (despliegue distribuido, service discovery, latencia de red).

# Best Practices

- Cada módulo debe tener una responsabilidad única y una interfaz clara hacia el resto del sistema.
- No introducir comunicación por red (HTTP, colas de mensajes) entre módulos internos: eso reintroduciría la complejidad de microservicios sin sus beneficios.

# Common Mistakes

- Diseñar módulos como si fueran a desplegarse por separado en el futuro ("por si acaso"), añadiendo complejidad innecesaria hoy.

# Hermes Usage

Ver [[ADR-001 - Modular Monolith]] para la decisión formal y su justificación ("Project simplicity"). Este patrón es compatible y complementario con [[Arquitectura en Capas de Hermes]].

# Related Notes

- [[ADR-001 - Modular Monolith]]
- [[Arquitectura en Capas de Hermes]]
- [[Alcance y Exclusiones]]

# References

- .ai/PROJECT.md (Priority 1)
- .ai/DECISIONS.md (Priority 1)
