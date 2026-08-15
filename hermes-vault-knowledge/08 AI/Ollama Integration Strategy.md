---
title: Ollama Integration Strategy
aliases: ["Sprint 6 Strategy", "LLM Pipeline", "OllamaClient Design"]
tags: [ai, hermes, ollama, architecture, sprint6]
status: stable
created: 2026-07-28
updated: 2026-07-29
source: Diseño de sesión (Claude Code) + implementacion y verificacion a escala real el mismo dia (1804 segmentos, entrevista real de ~70 min)
related: ["ADR-005 - Ollama como Motor LLM", "ADR-015 - cpp-httplib como Cliente HTTP para Ollama", "whisper.cpp Architecture", "GPU Acceleration Strategy", "Sprint 5 - Whisper Integration", "Sprint 6 - Ollama Integration", "Filosofia de Repositorios", "Privacy First"]
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

## Fase 3 — Resúmenes (opcional, decisión 2026-07-29)

Patrón *map-reduce* estándar: resumir cada bloque del transcript ya corregido (Fase 1) y anonimizado (Fase 2) — resumir después de anonimizar, no antes, para que el resumen mismo no filtre PII — y despues resumir el conjunto de resúmenes de bloque en un resumen final.

**Confirmado por el owner del proyecto: el resumen no es un entregable esperado del programa** (a diferencia de lo que decía la fuente original en `Sprint 6 - Ollama Integration.md`) — solo la transcripción corregida y anonimizada lo es. Implementado como **opt-in explícito**:

- `Job.includeSummary` (`backend/jobs/include/Job.h`), default `false`.
- `POST /interview/:id/process` acepta body opcional `{"include_summary": true}` — sin body, JSON invalido, o el campo ausente, default `false` (no es un error de la request).
- `TranscriptEnhancer::enhance(segments, includeSummary)` ni siquiera ejecuta la Fase 3 si `includeSummary` es `false` — no se gastan las llamadas a Ollama de esa fase (~30% del total de llamadas en la corrida real de 1804 segmentos).
- Si no se pidió resumen, `interview_results.summary_file_path` queda `NULL` — es el caso normal, no un fallo.
- Frontend: checkbox "Generar resumen" en la página de detalle de la entrevista, con aviso de que puede aumentar el tiempo de procesamiento en ~30% (relativo, no un tiempo fijo en minutos — la duración real depende del largo de la entrevista, que no se conoce hasta que termina de transcribir).

## Infraestructura compartida

- **`ILLMClient`/`OllamaClient`** (ver [[Filosofia de Repositorios]]), mapea a `POST /api/chat` con `"stream": false` (ver [[Ollama - Documentacion Oficial]]) — respuesta JSON completa en una sola llamada bloqueante, consistente con que el worker de Sprint 4 ya es síncrono (un thread, una tarea a la vez).
- Cliente HTTP: **cpp-httplib** (header-only, vía vcpkg, sin TLS/compresión — Ollama corre en `localhost` plano). Ver [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]].
- Modelo: familia Qwen (ADR-005), tamaño configurable por env var (mismo patrón que `WHISPER_MODEL_PATH`) — punto de partida sugerido `qwen2.5:7b-instruct`, con `qwen2.5:3b-instruct` como alternativa más liviana si el hardware del researcher no da abasto. Igual que con los modelos de whisper, el researcher lo descarga/pull a mano (`ollama pull qwen2.5:7b-instruct`), no se automatiza.

## Manejo de errores y degradación

Si Ollama no está corriendo, no tiene el modelo, o falla a mitad de las fases: el `transcript_final.txt` sin diarizar de Sprint 5 **debe seguir siendo el resultado disponible**, no bloquear la entrevista completa. Mismo criterio que la carga perezosa del modelo de whisper — la ausencia de una capacidad de IA no debería tumbar el pipeline entero, solo esa mejora puntual. Esto es más delicado con la anonimización: si falla, **no se debería exportar** ese resultado (Sprint 7 tiene que verificar explícitamente que la entrevista pasó por anonimización antes de permitir exportar) — a diferenciar claramente de un fallo en corrección/resumen, que sí puede degradarse con gracia.

**Ampliación tras la prueba a escala real**: no alcanza con que la Fase 2 "no haya fallado" (sin excepción) para considerar una entrevista segura de exportar — el recall incompleto de entidades (ver Common Mistakes) significa que Ollama puede "completar exitosamente" la anonimización y aun así dejar PII real sin cubrir. Sprint 7 necesita más que un chequeo booleano de "¿corrió Ollama sin error?"; probablemente una advertencia explícita al usuario de que la anonimización automática no es infalible y amerita revisión antes de compartir el export.

## Arquitectura del job — pregunta abierta

¿Estas fases corren todas dentro del mismo `execute()` de `InterviewProcessingJobHandler` (extendiéndolo, como Sprint 5 extendió el stub de Sprint 4), o se separan en jobs/fases propias (permitiendo, por ejemplo, re-anonimizar con otro modelo sin re-transcribir)? Mismo tipo de decisión que ya se tomó deliberadamente **no** resolver en Sprint 5 (no se agregó el estado `transcribed` intermedio porque nada lo consumía todavía) — ahora sí hay un consumidor real (Sprint 6), así que vale la pena reabrir esa decisión antes de implementar.

# Why it matters

La anonimización es el entregable de Hermes más directamente ligado a su propuesta de valor (Privacy First) — sin este diseño resuelto, [[Sprint 7 - Export]] no tiene una base segura sobre la cual exportar.

# Best Practices

- Chunkear por cantidad de segmentos (no por minutos de audio): el volumen real de texto por segmento es más estable que la duración, y es lo que efectivamente ocupa la ventana de contexto.
- Anonimizar con tabla de sustitución construida sobre la entrevista completa, nunca bloque por bloque de forma independiente — la consistencia importa más que la velocidad acá.
- Resumir después de anonimizar, nunca antes.
- Ollama en `localhost` sin TLS: no hace falta linkear `openssl` en `cpp-httplib` — mantiene el build liviano (mismo criterio que FFmpeg con features mínimas, ver [[ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio]]).

# Common Mistakes

- Asumir que la atribución de hablante por LLM sobre texto plano es diarización real — es una aproximación heurística, con errores esperables en diálogo rápido/interrumpido. Comunicarlo así al usuario. **Confirmado en la practica**: en la corrida real de 1804 segmentos, el modelo dejo de aplicar la etiqueta `Investigador:`/`Entrevistado:` en algunas lineas de bloques mas avanzados — no es 100% consistente en cada bloque.
- **Encontrado el 2026-07-29, evidencia directa - la atribucion de hablante NO es determinística entre corridas del mismo audio.** Se proceso el mismo archivo (`LOS AMIGOS DE EDU CRISTIANO RONALDO ENTREVISTA COMPLETA.mp3`) dos veces (interview_id=1 y interview_id=8, mismo modelo `qwen2.5:7b`). Comparando linea por linea, varios tramos quedaron con etiquetas distintas entre las dos corridas para el mismo contenido:
  ```
  Corrida 1 (interview_id=1):        Corrida 2 (interview_id=8):
  Entrevistado: ¿Cómo estás,          Entrevistado: ¿Cómo estás, hermano?
    hermano? Bien.                   Investigador: Bien.
  Entrevistado: ¿Qué tal en Arabia?   Investigador: ¿Qué tal en Arabia?
  Investigador: Muy bien.             Entrevistado: Muy bien.
  ```
  Es decir, ni siquiera es consistentemente incorrecto de la misma manera - el LLM literalmente decide distinto cada vez para las mismas frases cortas ("Bien.", "Muy bien.") donde no hay señal gramatical clara de quién habla. Esto confirma que el problema no es solo "diálogo rápido es dificil" (ya documentado) sino que **el mecanismo en sí no es confiable para frases cortas sin contexto semántico fuerte** - es la causa mas probable de lo que el usuario reporto como "una frase del entrevistado queda atribuida al entrevistador".
- **Encontrado el 2026-07-29**: el modelo a veces no respeta las dos etiquetas exactas pedidas en el prompt (`Investigador:`/`Entrevistado:`) y emite variantes inventadas - se vieron `Investigado:` y `Entrevistador:` en al menos dos entrevistas reales distintas (interview_id=8 e interview_id=9), no un caso aislado. A diferencia del problema de atribución (semánticamente difícil), esto es un bug de adherencia al formato, más barato de mitigar: normalizar la salida con una pasada de post-procesamiento (regex/mapeo de variantes conocidas a las dos etiquetas canónicas) antes de guardar `transcript_final.txt`, o reforzar el prompt con ejemplos few-shot del formato exacto. **Resuelto 2026-08-15 para variantes cercanas, ver Backlog de calidad, item 1.**
- **Encontrado el 2026-08-15, no resuelto a propósito**: en interview_id=10 apareció la etiqueta `Investervistado:` — a distancia de edición 6 tanto de `investigador` como de `entrevistado` (empate exacto, calculado con Levenshtein). El post-procesamiento del item 1 (umbral ≤3) correctamente no la toca: ampliar el umbral para cubrir este caso implicaría adivinar a ciegas cuál de los dos hablantes es, con 50% de probabilidad de asignarlo mal de forma silenciosa — peor que dejarlo visible como está, porque al menos así un revisor humano nota que algo salió mal. Queda como hallazgo abierto, no como bug a parchear con el mismo mecanismo que el item 1.
- **Encontrado el 2026-07-29**: whisper.cpp puede alucinar texto en otro idioma en tramos de audio poco claros/silenciosos - se vio una línea completa en chino (`Practic平稳过渡到下一个问题：`) en medio de una transcripción en español (interview_id=9), que pasó sin corregirse por la Fase 1 de Ollama. Sugiere agregar una validación simple (¿el texto del segmento usa mayoritariamente el alfabeto esperado para el idioma forzado?) antes de pasarlo a Ollama, en vez de confiar en que la fase de corrección lo arregle.
- Intentar mandar los 1804 segmentos (u equivalente) de una entrevista real en un solo prompt — no entra en la ventana de contexto de un modelo local razonable.
- Anonimizar cada bloque de forma aislada sin una tabla de sustitución compartida — genera inconsistencias (`"Cristiano"` anonimizado distinto en cada bloque), rompiendo la promesa de Privacy First.
- **Encontrado en la corrida real, hallazgo critico**: la tabla de sustitucion de dos pasadas es consistente para lo que detecta (la misma entidad recibe siempre el mismo placeholder), pero el **recall** del prompt de extraccion (Fase 2a) es incompleto — en la entrevista de prueba, nombres de figuras publicas mencionadas de pasada ("Messi", "Pele", "Maradona", "Florentino Perez", "Pepe") **no se anonimizaron**, y aparecieron sin tocar tanto en el transcript corregido como en el resumen final (que se genera a partir del texto ya anonimizado, heredando el hueco). **La anonimizacion actual NO esta lista para confiar en ella sin revision humana** - Sprint 7 (Export) no deberia asumir que una entrevista "anonimizada" esta realmente libre de PII sin ese chequeo adicional. Pendiente: refinar el prompt de extraccion (ejemplos few-shot, pedir explicitamente nombres de personas publicas/famosas), evaluar una pasada de verificacion extra, o mantener una lista de nombres conocidos del dominio (jugadores, clubes) como refuerzo determinístico ademas del LLM.

# Backlog priorizado de calidad (2026-07-29, items 1 y 2 implementados 2026-08-15)

Los 4 hallazgos de arriba no son igual de costosos de resolver. Orden recomendado, decidido antes de cerrar sesión para retomar en la próxima:

**1. Normalizar variantes de etiqueta** (barato, bajo riesgo) — ✅ **Implementado 2026-08-15**. `TranscriptEnhancer::correctAndStructure()` normaliza cada línea de cada bloque con `normalizeSpeakerLabel()`: si la palabra antes de `:` no es exactamente `Investigador`/`Entrevistado`, se mide distancia de edición (Levenshtein) contra ambas etiquetas canónicas y, si la distancia es ≤3, se reemplaza por la más cercana (ej. `Investigado:` → `Investigador:`, `Entrevistador:` → `Entrevistado:`). Post-procesamiento simple, sin tocar el prompt ni volver a invocar a Ollama. Se aplica antes de calcular la continuidad entre bloques, para que esa continuidad también quede con etiquetas canónicas.

**2. Determinismo entre corridas** (barato, gratis en esfuerzo) — ✅ **Implementado 2026-08-15**. `OllamaClient::chat()` ahora fija `"options": {"temperature": 0, "seed": 42}` en el body de `/api/chat` (antes no fijaba nada, así que usaba el default de Ollama, ~0.8, bastante aleatorio). Esto no arregla que la atribución sea *correcta*, pero la vuelve *consistente* — mismo audio, mismo resultado siempre. No soluciona el hallazgo del recall de anonimización (eso es un problema de qué detecta el modelo, no de aleatoriedad). **Pendiente de verificar**: no se re-corrió la misma entrevista (interview_id=1/8) dos veces todavía para confirmar en la práctica que el resultado es ahora idéntico bit a bit entre corridas — la garantía de determinismo depende de que Ollama respete `seed` de forma consistente para el modelo `qwen2.5:7b`, lo cual no se auditó a fondo, solo se aplicó siguiendo la API documentada.

**3. Validación de idioma/alfabeto** (esfuerzo medio) — antes de mandar cada segmento de whisper a Ollama, chequear que el texto use mayoritariamente el alfabeto esperado para el idioma forzado (`WHISPER_LANGUAGE`); si no, marcarlo o descartarlo en vez de dejar pasar alucinaciones como la línea en chino encontrada en interview_id=9.

**4. Recall de extracción de entidades** (esfuerzo alto, el más importante — bloquea [[Sprint 7 - Export]]) — necesita iteración real: prompt con ejemplos few-shot, posible lista determinística de nombres conocidos del dominio como refuerzo, y varias corridas de prueba para medir mejora real. Merece su propia sesión dedicada, no un ajuste de pasada.

Recomendación: 1 y 2 juntos en la próxima sesión (ambos son cambios chicos y acotados), 3 después, 4 aparte. Ver también [[GPU Acceleration Strategy]], documentado el mismo día como backlog paralelo (rendimiento, no calidad) — ninguno de los dos bloquea al otro.

# Hermes Usage

Implementado y verificado a escala real el 2026-07-28/29: `OllamaClient`/`TranscriptEnhancer` corridos contra entrevistas reales (interview_id=1, 8, 9), con logging de progreso agregado el 29 (ver whisper.cpp Architecture y Sprint 4 para el patrón de `current_step`). El transcript corregido y el resumen final son de calidad genuinamente utilizable como *lectura*, pero la atribución de hablante línea por línea **no es confiable todavía** — no determinística entre corridas del mismo audio (ver Common Mistakes). **Pendiente antes de confiar en producción**: (1) recall de la extracción de entidades incompleto, (2) atribución de hablante inconsistente entre corridas, (3) el modelo a veces no respeta el formato exacto de etiquetas pedido. Ninguno de los tres bloquea usar la transcripción como texto plano — si bloquean tratar la salida como una diarización o anonimización confiables sin revisión humana. Extiende el pipeline de [[Sprint 5 - Whisper Integration]] sin tocar `IJobQueue`/`WorkerPool`, con degradación con gracia verificada (el `transcript_final.txt` de Sprint 5 nunca se pierde).

# Related Notes

- [[ADR-005 - Ollama como Motor LLM]]
- [[ADR-015 - cpp-httplib como Cliente HTTP para Ollama]]
- [[whisper.cpp Architecture]]
- [[GPU Acceleration Strategy]]
- [[Sprint 5 - Whisper Integration]]
- [[Sprint 6 - Ollama Integration]]
- [[Sprint 7 - Export]]
- [[Privacy First]]
- [[Filosofia de Repositorios]]
- [[Ollama - Documentacion Oficial]]

# References

- Sesión de diseño 2026-07-28/29 (Priority 1, este documento — implementado, backlog de calidad y GPU pendiente)
- Datos reales de Sprint 5: entrevista de ~70 minutos, 1804 segmentos (interview_id=1, verificado end-to-end)
- https://github.com/ollama/ollama/blob/main/docs/api.md (Priority 4)
