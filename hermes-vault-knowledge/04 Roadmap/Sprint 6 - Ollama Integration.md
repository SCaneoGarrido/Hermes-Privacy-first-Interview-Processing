---
title: Sprint 6 - Ollama Integration
aliases: []
tags: [roadmap, sprint, hermes]
status: draft
created: 2026-07-24
updated: 2026-07-29
source: README.md — corregido 2026-07-29 con alcance real confirmado directamente por el owner del proyecto (ver nota de alcance mas abajo)
related: ["Sprint 5 - Whisper Integration", "Sprint 7 - Export", "ADR-005 - Ollama como Motor LLM", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama", "Ollama Integration Strategy"]
---

# Summary

Integración de IA local (Ollama) para corrección ortográfica, puntuación y anonimización. El resumen es una función opcional, no un entregable esperado del programa.

# Explanation

**Objetivo:** integración IA local.

**Entregables requeridos:** cliente HTTP hacia Ollama, corrección ortográfica, puntuación, anonimización.

**Opcional (no requerido por el programa)**: resúmenes — la nota original de esta sección (fuente: README.md) lo listaba como cuarto entregable junto a los demás, pero el owner del proyecto confirmó el 2026-07-29 que **no** forma parte de lo que el programa espera: solo se necesita la transcripción (corregida + anonimizada). Se implementó como funcionalidad opt-in: el usuario la activa desde un checkbox en el frontend al disparar `/process`, con aviso de que aumenta el tiempo de procesamiento en ~30%. Por defecto queda apagada. Si `README.md` todavía lista resúmenes como entregable central, esa fuente Priority 2 quedó desactualizada frente a esta confirmación directa — vale la pena corregirla ahí también.

> **Implementado y verificado a escala real el 2026-07-28/29** — ver [[Ollama Integration Strategy]] y [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]] (ambos `status: stable`/`accepted`). Corrido contra los 1804 segmentos reales de una entrevista de ~70 min: transcript corregido y resumen de calidad genuinamente utilizable, sin crashes, ~11 minutos de procesamiento (transcripción + anonimización + resumen opcional incluido). **Pendiente antes de confiar en producción**: la anonimización tiene recall incompleto — nombres de figuras públicas mencionadas de pasada no siempre se detectan y quedan sin anonimizar. No tratar la anonimización automática como infalible hasta refinar el prompt de extracción de entidades.

# Why it matters

La anonimización es el entregable más directamente ligado al principio [[Privacy First]]: convierte una transcripción cruda en un documento seguro de compartir.

# Best Practices

- Aislar el cliente de Ollama detrás de `ILLMClient` (ver [[Filosofia de Repositorios]]) para poder cambiar de modelo sin tocar el dominio.

# Common Mistakes

- Asumir que la anonimización del LLM es 100% infalible sin validación o revisión adicional antes de exportar.
- Asumir que el resumen se genera siempre — es opt-in (`include_summary` en el body de `POST /interview/:id/process`, default `false`). Un `summary_file_path` en `null` en la respuesta es el caso normal, no un error.

# Hermes Usage

Cierra el pipeline de procesamiento de entrevistas antes de la exportación en [[Sprint 7 - Export]].

# Related Notes

- [[Sprint 5 - Whisper Integration]]
- [[Sprint 7 - Export]]
- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]
- [[Ollama Integration Strategy]]

# References

- README.md (Priority 2)
