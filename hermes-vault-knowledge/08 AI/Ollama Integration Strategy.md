---
title: Ollama Integration Strategy
aliases: ["Sprint 6 Strategy", "LLM Pipeline", "OllamaClient Design"]
tags: [ai, hermes, ollama, architecture, sprint6, draft]
status: draft
created: 2026-07-28
updated: 2026-07-28
source: Diseño de sesión (Claude Code, pre-implementación de Sprint 6, pendiente de aprobación humana) — informado por datos reales de Sprint 5 (entrevista real de ~70 min, 1804 segmentos)
related: ["ADR-005 - Ollama como Motor LLM", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama", "whisper.cpp Architecture", "Sprint 5 - Whisper Integration", "Sprint 6 - Ollama Integration", "Filosofia de Repositorios", "Privacy First"]
---

# Summary

Estrategia para las cuatro tareas de Sprint 6 (corrección ortográfica/puntuación, estructuración por hablante, resúmenes, anonimización) sobre `transcript_raw.json` (Sprint 5), resolviendo el problema real descubierto con datos de producción: una entrevista de 70 minutos genera **1804 segmentos** — demasiado para un único prompt a un LLM local.

# Explanation

## El problema que cambia el diseño original

El diseño de Sprint 5 ([[whisper.cpp Architecture]]) sugería mandarle todo el JSON crudo a Ollama en un solo prompt. Con datos reales (ver [[Sprint 5 - Whisper Integration]]) eso no es viable: 1804 segmentos superan la ventana de contexto de los modelos Qwen que corren razonablemente en CPU sin GPU. Cualquier tarea de Sprint 6 tiene que procesar el transcript **en bloques**, no de una sola vez.

## Alcance real de Sprint 6 (por roadmap, no solo diarización)

`Sprint 6 - Ollama Integration.md` documenta cuatro entregables, con prioridad implícita distinta:

1. **Corrección ortográfica y puntuación** (incluye estructurar por turno de hablante — el raw de whisper no tiene esto, es audio mono de un solo canal).
2. **Resúmenes**.
3. **Anonimización** — el entregable más importante: es lo que liga directamente a [[Privacy First]] y bloquea [[Sprint 7 - Export]] (no se exporta una entrevista sin anonimizar primero).

Estas tres tareas tienen necesidades de chunking distintas — no se resuelven con la misma estrategia:

- **Corrección/estructuración**: transforma el texto en sí, necesita ver el transcript completo pero puede procesarse secuencialmente en bloques con continuidad de contexto entre bloques (ver abajo).
- **Resumen**: patrón *map-reduce* clásico para documentos largos — resumir cada bloque, después resumir el conjunto de resúmenes. No necesita ver todo de una vez.
- **Anonimización**: el caso más difícil. Necesita **consistencia entre bloques** — si "Cristiano" se anonimiza como `[PERSONA_1]` en el bloque 3, tiene que ser `[PERSONA_1]` también en el bloque 40, no `[PERSONA_2]`. Esto no se resuelve con chunking ingenuo (ver Fase 3 abajo).

## Fase 1 — Corrección + estructuración por hablante (habilita las otras dos)

Lee `transcript_raw.json` (path guardado en `interview_jobs.raw_transcript_path`, ver [[whisper.cpp Architecture]]) en bloques de ~50-80 segmentos (suficiente margen bajo cualquier ventana de contexto razonable, incluso en modelos chicos). Por cada bloque:

- Se le pasa al LLM el bloque actual **más las últimas 2-3 líneas ya corregidas/etiquetadas del bloque anterior** como contexto de continuidad (para que sepa quién habló último y no reinicie la suposición de que el primer turno es "Investigador" en cada bloque).
- El prompt pide: puntuación/ortografía corregida + etiqueta de hablante (`Investigador:`/`Entrevistado:`) basada en contexto gramatical y semántico — el enfoque original de [[whisper.cpp Architecture]], ahora explícitamente por bloques.
- Los resultados de cada bloque se concatenan en orden para formar el `transcript_final.txt` definitivo, **reemplazando** el contenido sin diarizar que dejó Sprint 5 (vía `IInterviewRepository::upsertTranscriptionResult`, ya es un UPSERT — no hace falta tocarlo).

**Limitación conocida, a documentar para el usuario final**: con diálogo real de intercambios muy cortos y rápidos (confirmado en la entrevista de prueba: "¿Cómo estás, hermano? Bien. ¿Qué tal todo? Todo bien y tú. Bien." — turnos de una a tres palabras, sin pausas claras) la atribución de hablante por LLM sobre texto solo (sin señal de audio/diarización acústica real) va a tener errores. No es 100% confiable y hay que comunicarlo así, no venderlo como diarización real — ver [[ADR-005 - Ollama como Motor LLM]], Common Mistakes.

## Fase 2 — Anonimización (la crítica para Privacy First)

**Estrategia de dos pasadas**, no una sola pasada por bloque:

1. **Pasada 1 — extracción de entidades**: recorrer el transcript ya corregido (Fase 1) en bloques, pidiéndole al LLM que liste nombres propios, lugares, empresas u otra información identificable por bloque. Acumular y deduplicar en una tabla de sustitución única para toda la entrevista (`"Cristiano" → "[PERSONA_1]"`, `"Al-Nassr" → "[ORGANIZACION_1]"`, etc.) construida **una sola vez**, con visión de la entrevista completa antes de decidir los reemplazos.
2. **Pasada 2 — sustitución**: aplicar la tabla de sustitución sobre el texto. Esto puede ser reemplazo de texto plano determinístico (más rápido, 100% consistente, sin volver a invocar al LLM) en vez de pedirle al LLM que anonimice bloque por bloque — evita exactamente el problema de inconsistencia entre bloques.

Esto es más trabajo de diseño que la Fase 1, pero es necesario: anonimizar bloque por bloque sin tabla compartida es una promesa de Privacy First que no se puede cumplir de forma confiable.

## Fase 3 — Resúmenes

Patrón *map-reduce* estándar: resumir cada bloque del transcript ya corregido (Fase 1) y anonimizado (Fase 2) — resumir después de anonimizar, no antes, para que el resumen mismo no filtre PII — y despues resumir el conjunto de resúmenes de bloque en un resumen final. Candidato a ser una acción bajo demanda (el usuario la pide desde el frontend) en vez de automática en el pipeline de `/process`, ya que no es indispensable para tener una entrevista "completa" y utilizable — a diferencia de la anonimización.

## Infraestructura compartida

- **`ILLMClient`/`OllamaClient`** (ver [[Filosofia de Repositorios]]), mapea a `POST /api/chat` con `"stream": false` (ver [[Ollama - Documentacion Oficial]]) — respuesta JSON completa en una sola llamada bloqueante, consistente con que el worker de Sprint 4 ya es síncrono (un thread, una tarea a la vez).
- Cliente HTTP: **cpp-httplib** (header-only, vía vcpkg, sin TLS/compresión — Ollama corre en `localhost` plano). Ver [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]].
- Modelo: familia Qwen (ADR-005), tamaño configurable por env var (mismo patrón que `WHISPER_MODEL_PATH`) — punto de partida sugerido `qwen2.5:7b-instruct`, con `qwen2.5:3b-instruct` como alternativa más liviana si el hardware del researcher no da abasto. Igual que con los modelos de whisper, el researcher lo descarga/pull a mano (`ollama pull qwen2.5:7b-instruct`), no se automatiza.

## Manejo de errores y degradación

Si Ollama no está corriendo, no tiene el modelo, o falla a mitad de las fases: el `transcript_final.txt` sin diarizar de Sprint 5 **debe seguir siendo el resultado disponible**, no bloquear la entrevista completa. Mismo criterio que la carga perezosa del modelo de whisper — la ausencia de una capacidad de IA no debería tumbar el pipeline entero, solo esa mejora puntual. Esto es más delicado con la anonimización: si falla, **no se debería exportar** ese resultado (Sprint 7 tiene que verificar explícitamente que la entrevista pasó por anonimización antes de permitir exportar) — a diferenciar claramente de un fallo en corrección/resumen, que sí puede degradarse con gracia.

## Arquitectura del job (decisión 2026-07-28)

Las tres fases corren dentro del mismo `execute()` de `InterviewProcessingJobHandler`, extendiéndolo — mismo patrón que Sprint 5 extendió el stub de Sprint 4, sin agregar tipos de job ni endpoints nuevos. Se descartó separar en fases/jobs propios (que habilitaría, por ejemplo, re-anonimizar con otro modelo sin re-transcribir) por ahora: es complejidad que nadie necesita todavía — mismo criterio que ya se aplicó para no agregar el estado `transcribed` intermedio en Sprint 5. Si en el futuro aparece una necesidad real de re-ejecutar una fase sola, se separa en ese momento.

# Why it matters

La anonimización es el entregable de Hermes más directamente ligado a su propuesta de valor (Privacy First) — sin este diseño resuelto, [[Sprint 7 - Export]] no tiene una base segura sobre la cual exportar.

# Best Practices

- Chunkear por cantidad de segmentos (no por minutos de audio): el volumen real de texto por segmento es más estable que la duración, y es lo que efectivamente ocupa la ventana de contexto.
- Anonimizar con tabla de sustitución construida sobre la entrevista completa, nunca bloque por bloque de forma independiente — la consistencia importa más que la velocidad acá.
- Resumir después de anonimizar, nunca antes.
- Ollama en `localhost` sin TLS: no hace falta linkear `openssl` en `cpp-httplib` — mantiene el build liviano (mismo criterio que FFmpeg con features mínimas, ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]).

# Common Mistakes

- Asumir que la atribución de hablante por LLM sobre texto plano es diarización real — es una aproximación heurística, con errores esperables en diálogo rápido/interrumpido. Comunicarlo así al usuario.
- Intentar mandar los 1804 segmentos (u equivalente) de una entrevista real en un solo prompt — no entra en la ventana de contexto de un modelo local razonable.
- Anonimizar cada bloque de forma aislada sin una tabla de sustitución compartida — genera inconsistencias (`"Cristiano"` anonimizado distinto en cada bloque), rompiendo la promesa de Privacy First.

# Hermes Usage

Diseño de referencia para implementar [[Sprint 6 - Ollama Integration]]. Extiende el pipeline de [[Sprint 5 - Whisper Integration]] (que ya deja `raw_transcript_path` y un `transcript_final.txt` sin diarizar como resultado utilizable en solitario). **Estado: draft, pendiente de aprobación humana antes de implementar** — mismo criterio que se siguió para Sprint 5.

# Related Notes

- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]
- [[whisper.cpp Architecture]]
- [[Sprint 5 - Whisper Integration]]
- [[Sprint 6 - Ollama Integration]]
- [[Sprint 7 - Export]]
- [[Privacy First]]
- [[Filosofia de Repositorios]]
- [[Ollama - Documentacion Oficial]]

# References

- Sesión de diseño 2026-07-28 (Priority 1, este documento — pendiente de aprobación)
- Datos reales de Sprint 5: entrevista de ~70 minutos, 1804 segmentos (interview_id=1, verificado end-to-end)
- https://github.com/ollama/ollama/blob/main/docs/api.md (Priority 4)
