# Prompts

System prompts used by `TranscriptEnhancer` (`backend/llm/src/TranscriptEnhancer.cpp`) for the three Sprint 6 phases. Keep this file in sync with the source -- it's the actual prompt text, not a paraphrase, so future edits should be made in both places together.

## Phase 1 -- Correction, per turn (`correctTurns`)

Since ADR-022 the LLM no longer assigns speakers. Speakers come from acoustic diarization (ADR-021), and this phase only corrects spelling and punctuation, turn by turn.

```
Sos un asistente que corrige transcripciones automaticas de entrevistas en espanol.
Recibis lineas numeradas con el formato '[n] (Hablante) texto'. Tu tarea: corregir solo
ortografia y puntuacion de cada linea (incluidos signos de pregunta). No cambies palabras por
sinonimos, no agregues ni quites contenido, no resumas, no unas ni dividas lineas y no agregues
comentarios propios. Devolve exactamente una linea por cada linea recibida, con el formato
'[n] texto corregido' (mismo numero, sin el hablante), sin explicaciones adicionales.
```

**User prompt.** Each chunk of 60 turns starts with `Ultimas lineas ya corregidas del fragmento anterior (solo contexto, no las devuelvas):` followed by the last 3 corrected lines. Then comes `Lineas a corregir:` and the indexed lines `[1] (Investigador) texto`, numbered from 1 within the chunk. The speaker shown is the role's display label: "Investigador" or the interview's `subject_type`.

**Parsing.** The code matches the response by index, not by line position, and strips a leading `(Hablante)` if the model repeats it. A turn keeps its **original** text in any of these cases:
- its index is missing from the response;
- the correction is empty;
- the length ratio of corrected to original falls outside 0.6-1.6. Turns shorter than 15 characters are exempt from the ratio check, but their correction must stay under 40 characters.

This replaces the free-text output that lost ~3% of segments at chunk boundaries.

**Fixed by ADR-022 (2026-10-05)**: the two known issues of the previous prompt (label variants such as `Investigado:`, and speaker attribution that changed between runs and inverted mid-interview) disappear because the model no longer emits labels. The ~3% content loss at chunk boundaries (2026-10-02) cannot happen anymore: missing lines fall back to the original. The new prompt has not yet been validated against a real interview.

## Phase 2a -- PII entity extraction (`anonymize`)

```
Sos un asistente que identifica informacion personal identificable (PII) en texto en espanol:
nombres de personas, lugares, organizaciones o empresas. Analiza el texto y devolve una lista,
una entidad por linea, en el formato exacto 'ENTIDAD|TIPO' donde TIPO es uno de: PERSONA, LUGAR,
ORGANIZACION, OTRO. No repitas la misma entidad mas de una vez. Si no encontras ninguna entidad,
no devuelvas nada. No agregues explicaciones ni texto fuera del formato pedido.
```

Run once per ~3000-char chunk of the corrected turn texts (one turn per line, **without speaker labels**, so labels cannot be extracted as entities). Results are accumulated into a single substitution table for the whole interview (see the two-pass design in the vault note). Phase 2b (substitution) is deterministic string replacement applied to each turn's text, not an LLM call.

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

Related, not an LLM prompt: whisper receives the same glossary as `initial_prompt`. The prompt is carried on every 30 s window (`carry_initial_prompt`) with no rolling context from the previous window, and is truncated to 150 tokens including the style sentence. Since ADR-020 it is followed by a fixed style sentence:

```
Glosario: a, b, c. Transcripción fiel de una entrevista, con puntuación, tildes y signos de pregunta: ¿cómo funciona? Bien, se lo explico.
```

Without keywords, only the style sentence is sent. It prevents whisper from drifting into unpunctuated text: on interview 6 it lost punctuation from minute 18 onward when it was conditioned on the previous window. No leakage of this text into transcripts was observed (2026-10-05).

## Model

`qwen2.5:7b` via Ollama, configurable with `OLLAMA_MODEL` (default) and `OLLAMA_BASE_URL` (default `http://localhost:11434`). `qwen2.5:3b` is a lighter fallback if `7b` is too slow on a given machine.
