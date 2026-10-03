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

**Known issue (2026-10-02)**: on a real 49-min interview, ~3% of the raw segments (15 of 450) were missing from the corrected text and two paragraphs were duplicated. Almost all of them sit at chunk boundaries (first ~15 or last segment of a 60-segment block): the model re-emits the continuity lines and skips new content. This is one of the reasons this phase became opt-in (`enhance_transcript`, ADR-017).

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

**Known issue (2026-10-02)**: severe over-substitution on a real interview:
- Common nouns were extracted as entities ("paciente" ~100 times, "médico", "compañera", "cáncer de próstata" as LUGAR), as were the speaker label `Investigador:` and public institutions (FONASA, GES, SIGGES, CESFAM, Superintendencia).
- The model also returned the type names themselves (`PERSONA`, `LUGAR`, `ORGANIZACION`) as entities. Because Phase 2b re-scans text that already contains placeholders, they nested (`[[ORGANIZACION_8]_4]`).
- It still missed a real person's name ("doctor Evans Perre").
- 6 of 15 extraction calls ran into `num_predict`, listing entities in a loop.

Made opt-in by ADR-017; fix pending (single-pass replacement, word boundaries, entity filtering, structured output).

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

## Glossary sanitization (`GlossarySanitizer::sanitize`)

```
Sos un asistente que revisa transcripciones automaticas de entrevistas en espanol. Recibis un glosario
de terminos correctos del tema de la entrevista y un fragmento transcripto. Tu tarea: encontrar palabras
o frases del fragmento que sean transcripciones erroneas de algun termino del glosario (suenan parecido
y el contexto lo confirma). Para cada una devolve 'original', copiado exactamente como aparece en el
fragmento, y 'termino', copiado exactamente del glosario. No propongas cambios para palabras que no
correspondan a un termino del glosario. Si no hay ninguna, devolve una lista vacia.
```

User message: `Glosario:\n- <termino>\n...\n\nFragmento:\n<segmentos, uno por linea>`.
- **Blocks:** 40 whisper segments per call.
- **Structured output:** the response is forced through Ollama's `format` field (JSON schema `{"correcciones": [{"original", "termino"}]}`), with `num_predict=512`.
- **The model only proposes; the code decides and replaces.** Each pair is validated deterministically:
  1. The term must be in the glossary.
  2. The original must appear as a whole word in the block.
  3. The original must not itself be a glossary term.
  4. Normalized edit distance ≤ 50%.

  Accepted pairs are replaced only inside that block, as whole words (ADR-018).

Related, not an LLM prompt: whisper receives the same glossary as `initial_prompt` (`Glosario: a, b, c.`, with `carry_initial_prompt`), truncated to 150 tokens.

## Model

`qwen2.5:7b` via Ollama, configurable with `OLLAMA_MODEL` (default) and `OLLAMA_BASE_URL` (default `http://localhost:11434`). `qwen2.5:3b` is a lighter fallback if `7b` is too slow on a given machine.
