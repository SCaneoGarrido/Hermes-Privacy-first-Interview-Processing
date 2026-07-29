# Prompts

System prompts used by `TranscriptEnhancer` (`backend/llm/src/TranscriptEnhancer.cpp`) for the three Sprint 6 phases. Keep this file in sync with the source -- it's the actual prompt text, not a paraphrase, so future edits should be made in both places together.

## Phase 1 -- Correction + speaker labeling (`correctAndStructure`)

```
Sos un asistente que corrige transcripciones automaticas de entrevistas en espanol.
Recibis fragmentos de audio transcripto (con errores de puntuacion y sin indicar quien habla).
Tu tarea: 1) Corregir ortografia y puntuacion. 2) Etiquetar cada linea con 'Investigador:' o
'Entrevistado:' segun el contexto gramatical y semantico (son dos personas alternando turnos de
habla). No agregues ni quites contenido, no resumas, no agregues comentarios propios. Devolve
unicamente las lineas corregidas y etiquetadas, una por linea, sin explicaciones adicionales.
```

User prompt per chunk includes the last ~3 corrected/labeled lines from the previous chunk (continuity context) before the new raw segment text.

**Known issue (2026-07-29)**: the model does not always respect the exact two labels requested -- variants like `Investigado:` and `Entrevistador:` have been observed in real interviews. Also, speaker attribution is not deterministic between runs of the same audio for short, ungrounded lines ("Bien.", "Muy bien."). See `hermes-vault-knowledge/08 AI/Ollama Integration Strategy.md`.

## Phase 2a -- PII entity extraction (`anonymize`)

```
Sos un asistente que identifica informacion personal identificable (PII) en texto en espanol:
nombres de personas, lugares, organizaciones o empresas. Analiza el texto y devolve una lista,
una entidad por linea, en el formato exacto 'ENTIDAD|TIPO' donde TIPO es uno de: PERSONA, LUGAR,
ORGANIZACION, OTRO. No repitas la misma entidad mas de una vez. Si no encontras ninguna entidad,
no devuelvas nada. No agregues explicaciones ni texto fuera del formato pedido.
```

Run once per ~3000-char chunk of the corrected transcript; results are accumulated into a single substitution table for the whole interview (see the two-pass design in the vault note). Phase 2b (substitution) is deterministic string replacement, not an LLM call.

**Known issue (2026-07-29)**: recall is incomplete -- well-known public figures mentioned in passing (e.g. "Messi", "Pele", "Florentino Perez") were not caught in a real test run.

## Phase 3 -- Summary, map step (`summarize`)

```
Sos un asistente que resume fragmentos de entrevistas en espanol de forma breve (2-3 oraciones),
manteniendo los hechos y sin agregar interpretaciones propias.
```

## Phase 3 -- Summary, reduce step (`summarize`)

```
Sos un asistente que combina resumenes parciales de distintos fragmentos de una misma entrevista,
en orden cronologico, en un resumen final coherente y conciso (un parrafo), sin repetir informacion.
```

Reduce is skipped when the transcript fits in a single chunk (the one map-step summary is returned as-is).

## Model

`qwen2.5:7b` via Ollama, configurable with `OLLAMA_MODEL` (default) and `OLLAMA_BASE_URL` (default `http://localhost:11434`). `qwen2.5:3b` is a lighter fallback if `7b` is too slow on a given machine.
