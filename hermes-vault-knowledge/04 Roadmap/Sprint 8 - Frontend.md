---
title: Sprint 8 - Frontend
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-10-02
source: README.md
related: ["Sprint 7 - Export", "Sprint 9 - Configuration", "API First", "ADR-011 - Contrato de Respuesta API Uniforme", "Sprint 4 - Background Processing"]
---

# Summary

Interfaz web en React para operar todo el pipeline de Hermes.

# Explanation

**Objetivo:** interfaz web.

**Entregables (actualizado 2026-08-15)**, adelantados fuera de orden respecto al roadmap original porque el objetivo inmediato era validar el flujo crear → subir audio → procesar de punta a punta:
- ✅ React + TypeScript + Vite, sin librería de UI ni autenticación (fuera de alcance v1.0)
- ✅ Carga de entrevistas (crear + asociar audio)
- ✅ Listado (`InterviewsListPage`, consume `GET /interviews`)
- ✅ Detalle (`InterviewDetailPage`, consume `GET /interview/:id`, incluye subir audio y botón "Enviar a procesar")
- ✅ Progreso — [[Sprint 4 - Background Processing]] terminó implementándose; el frontend hace polling del job real, mostrando paso actual (`normalizando_audio`/`transcribiendo`/`corrigiendo_texto`/`anonimizando`/`generando_resumen`) y tiempo transcurrido
- ✅ Checkbox opcional "Generar resumen" (`include_summary`), con aviso de que aumenta el tiempo de procesamiento
- ✅ Descargas — `<a download>` directo a `/interview/:id/download/transcript` y `/download/summary` (no pasa por el cliente JSON, ver comentario en `frontend/src/api/interviews.ts`)
- ❌ Configuración — depende de [[Sprint 9 - Configuration]], todavía no implementado; toda la config del backend es por variables de entorno

**Actualización 2026-10-02 — rediseño y vista de lectura:**
- ✅ **Identidad visual griega clásica** a partir de dos mockups (`docs/img/mockups/`): negro ático, terracota, marfil, azul Egeo y oro viejo; títulos epigráficos con serif del sistema (Palatino Linotype/Georgia); meandro como divisor; isotipo "H" con ala de talaria; íconos SVG propios. Modo oscuro y claro definidos (el claro sin revisión visual). Sin dependencias nuevas ni recursos de red.
- ✅ **Lista** ("Corpus de entrevistas"): contadores, búsqueda/orden/filtros por estado en el cliente, tarjetas con una acción principal por estado, modal propio para borrar, refresco automático mientras algo procesa.
- ✅ **Detalle por estado**: ficha, "secuencia de umbrales" I–VI para los pasos, zona de arrastrar y soltar con validación previa, confirmación al reemplazar audio, avisos de revisión junto a las descargas.
- ✅ **Vista de lectura** `/interviews/:id/transcript` (ADR-019 en `.ai/DECISIONS.md`): portada con ficha y aviso de revisión, resumen, turnos por hablante o párrafos con marca de tiempo, panel con scroll propio, búsqueda con salto a la coincidencia, marcadores `[PERSONA_1]` resaltados, "Imprimir / Guardar PDF" con hoja de estilos de impresión.
- Se descartaron del mockup los elementos sin dato real en la API (métricas inventadas, "cancelar procesamiento", consola de CPU, afirmaciones de "aislamiento criptográfico").
- Limitación conocida: la API no dice con qué opciones se lanzó un job; el navegador recuerda las de los jobs que él lanzó y, si no las conoce, muestra los pasos opcionales como "Si se pidió".

# Why it matters

Es el único punto de interacción humana con el sistema; debe consumir la REST API existente sin introducir lógica de negocio propia ([[API First]]).

# Best Practices

- El frontend solo orquesta llamadas a la API y presenta estado (`pending_audio` / `pending_processing` / `processing` / `completed` / `failed`, ver [[Sprint 4 - Background Processing]]).
- Usar un proxy de dev server (`vite.config.ts`) hacia el backend en vez de configurar CORS, mientras backend y frontend se sirvan desde el mismo origen en producción.
- Diseñar el cliente HTTP para tolerar endpoints que todavía no existen en el backend (ver [[ADR-011 - Contrato de Respuesta API Uniforme]]): mostrar "no disponible" en vez de romper la UI.

# Common Mistakes

- Implementar validaciones de negocio (por ejemplo, reglas de anonimización) en el cliente React en vez de en el backend.

# Hermes Usage

Consume los endpoints documentados en `docs/API_REQUIREMENTS.md` — el contrato completo (qué existe, qué falta, y por qué) vive ahí en vez de en el código del frontend únicamente. El detalle técnico completo de la API que el frontend consume está en [[Sprint 2 - Persistence]] y [[Sprint 3 - File Upload]].

# Related Notes

- [[Sprint 7 - Export]]
- [[Sprint 9 - Configuration]]
- [[API First]]
- [[ADR-011 - Contrato de Respuesta API Uniforme]]
- [[Sprint 4 - Background Processing]]

# References

- README.md (Priority 2)
- docs/API_REQUIREMENTS.md (Priority 1)
