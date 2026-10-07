# Architectural Decisions

## ADR-001

Architecture

Decision

Modular Monolith

Reason

Project simplicity.

---

## ADR-002

Language

Decision

C++20

Reason

Learning objectives and performance.

---

## ADR-003

REST Framework

Decision

Crow

Reason

Minimalistic framework similar to Express and Flask.

---

## ADR-004

Speech Recognition

Decision

whisper.cpp

Reason

Native C++ implementation.

---

## ADR-005

LLM

Decision

Ollama

Reason

Offline execution.

---

## ADR-006

Database

Decision

SQLite

Reason

No server required.

Status: Superseded by ADR-008 (2026-07-28).

---

## ADR-007

Cloud

Decision

Forbidden by default.

Reason

Medical interview privacy.

---

## ADR-008

Database

Decision

MySQL, running in Docker (docker-compose.yml), superseding ADR-006 (SQLite).

Reason

The project decided early to run against a robust, server-based database instead of migrating later, and to run it in Docker so every contributor has an identical local environment. See docs/decisions/0001-cliente-base-de-datos.md for the full writeup.

---

## ADR-009

MySQL Client Library

Decision

libmariadb (MariaDB Connector/C), not mysql-connector-cpp.

Reason

mysql-connector-cpp's `jdbc` feature (classic MySQL protocol) depends on the vcpkg port `libmysql`, which explicitly does not support MinGW (`"supports": "!android & !mingw & !uwp & !xbox"`) -- the project's toolchain has no MSVC installed. The X DevAPI (mysql-connector-cpp without `jdbc`) does build on MinGW but pulls a heavy dependency chain (protobuf, openssl, rapidjson) and talks a non-standard protocol (X Protocol, port 33060). libmariadb builds on MinGW, has a much lighter dependency footprint (zlib only), and speaks the standard MySQL protocol on port 3306 -- prioritized because the project intends to be easy for outside contributors to clone and build. See docs/decisions/0001-cliente-base-de-datos.md.

---

## ADR-010

Local Database Runtime

Decision

Docker Compose, not a natively installed MySQL server.

Reason

Guarantees every contributor runs the same MySQL version with the same configuration, avoids environment drift between "works on my machine" and the eventual deployment target, and is trivial to reset during development (`docker compose down -v`).

---

## ADR-011

API Response Contract

Decision

Every HTTP response (success, validation failure, 404, 405, and unhandled exceptions) uses the same JSON envelope: `{ "success": bool, "data": <object|array|null>, "error": {"code": "SCREAMING_SNAKE_CASE", "message": string} | null }`.

Reason

Gives the frontend a single, predictable shape to parse regardless of endpoint or failure mode, instead of ad-hoc error bodies per route. Enforced via `ApiResponse` helper, `CROW_CATCHALL_ROUTE` (404/405), and `app.exception_handler` (uncaught exceptions). See backend/api/rules/contract.md.

---

## ADR-012

Repository & Service Layer for Interviews

Decision

Introduce `IInterviewRepository` (implemented by `MySqlInterviewRepository`) and a thin `InterviewService` between the controllers and the database. `InterviewController` and `FileController` now depend only on `InterviewService`; neither calls `DatabaseManager` directly.

Reason

Closes the deviation from the Repository Philosophy accepted during Sprint 2 ("move fast on the create -> upload -> process flow first, add the interface later") before Sprint 4 (Background Processing) introduces worker threads and more call sites -- retrofitting the abstraction later, with more code touching the database, would have been more expensive. `InterviewService` has no dependency on Crow or MySQL, keeping business rules (e.g. audio required before processing, deleting the on-disk audio file when an interview is removed) out of both the controller and the repository.

---

## ADR-013

API Versioning

Decision

Prefix every backend route with `/api/v1` (e.g. `/api/v1/interviews`, `/api/v1/interview/:id`). `frontend/src/api/client.ts` now builds requests against `BASE_URL = "/api/v1"`, and the Vite dev proxy (`frontend/vite.config.ts`) forwards `/api/*` to the backend without rewriting the path, since the backend already serves that exact prefix.

Reason

Sprint 1 - Core API originally called for versioning the API from that sprint onward, but it was deliberately deferred (see Sprint 2 - Persistence discussion) to keep the create -> upload -> process flow moving. Adopted now, alongside the repository/service cleanup of ADR-012, while the route surface is still small. Without updating the frontend `BASE_URL` and the Vite proxy rewrite in the same change, every frontend request would 404 against the newly prefixed backend routes -- both were changed together here.

---

## ADR-014

Audio Normalization

Decision

FFmpeg (avcodec, avformat, swresample only -- no encoders, no GPL codecs), linked statically via vcpkg, wrapped behind `IAudioNormalizer`. Never invoked as an external `ffmpeg.exe` process.

Reason

whisper.cpp requires PCM 16-bit/16kHz/mono; Sprint 3 already accepts wav/ogg/m4a/mp3. Shelling out to a system `ffmpeg` binary would force every contributor to install and PATH it manually before they could even build the project -- the same build-friendliness criterion already applied in ADR-009. Static linking keeps "clone and `cmake --build`" true with zero extra manual steps. See `hermes-vault-knowledge/03 ADR/ADR-014 - FFmpeg Estatico via vcpkg para Normalizacion de Audio.md` for the platform gotchas found in practice (vcpkg's `FindFFmpeg.cmake` module, not namespaced targets; the `CMAKE_BUILD_TYPE` bug this exposed).

---

## ADR-015

HTTP Client for Ollama

Decision

cpp-httplib (header-only, vcpkg, no TLS/compression features), wrapped behind `ILLMClient`/`OllamaClient`. Talks to Ollama's local REST API (`POST /api/chat`, `"stream": false`) over plain HTTP on `localhost`.

Reason

Crow is server-only; Sprint 6 needs an outbound HTTP client. cpp-httplib is header-only (near-zero build cost, unlike FFmpeg/whisper.cpp) and sufficient for a single blocking request at a time, matching the already-synchronous worker from Sprint 4. libcurl and Boost.Beast were considered and rejected as unnecessary complexity for local, TLS-free traffic. See `hermes-vault-knowledge/03 ADR/ADR-015 - cpp-httplib como Cliente HTTP para Ollama.md` for the MinGW build gotchas found in practice (`_WIN32_WINNT`, `CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO`).

---

## ADR-016

Video Input (.mp4)

Decision

Accept `video/mp4` uploads and extract the audio with the existing `FfmpegAudioNormalizer` inside the processing job -- no new component, no new dependency. Once extracted, the WAV is copied to `uploads/<uuid>.wav`, `interviews_audio` is pointed at it (`IInterviewRepository::updateAudio`), and only then is the original video deleted. Uploads are capped at 1 GiB (`Config::MAX_UPLOAD_BYTES`, `413 PAYLOAD_TOO_LARGE`), checked in `FileFormatGuard` before multipart parsing and mirrored in the frontend before transfer.

Reason

Some interviews are recorded as video (Zoom/Meet). libavformat already opens MP4 with the same demuxer used for `.m4a`, and `av_find_best_stream(AUDIO)` ignores the video track, so a separate video-to-audio converter at upload time was rejected: it would duplicate the normalizer and block the HTTP thread on a long decode. The video is not retained because faces are PII that the pipeline never uses -- keeping it would only enlarge what sits on disk. The order copy -> DB update -> delete keeps the DB from ever pointing at a missing file, and the copy (rather than reusing `storage/interviews/<id>/audio.wav`) keeps reprocessing from reading and writing the same file. The 1 GiB cap exists because Crow buffers the whole body and the multipart is copied several times in memory; videos of several GB (phone 1080p) would need streaming upload, deliberately out of scope here. `.mov` (`video/quicktime`) shares the `ftyp` signature and can be added later the same way.

---

## ADR-017

LLM Correction and Anonymization Become Opt-In

Decision

`POST /interview/:id/process` takes a new optional flag `enhance_transcript` (default `false`). Without it, the delivered `transcript_final.txt` is whisper's plain output -- one segment per line, no speaker labels, no placeholders -- and Ollama is not called at all. Correction + speaker labeling + anonymization (Sprint 6 phases 1-2) run only when the flag is set. The summary (`include_summary`) stays independent and is always a separate document (`transcript_summary.txt`): with `enhance_transcript` off it summarizes the plain whisper text; with it on, the anonymized text (and is skipped if anonymization was requested but failed). The `[AVISO HERMES] ... NO fue anonimizada` header is written only when anonymization was requested and failed.

Reason

Deep manual validation of a real 49-min interview (2026-10-02, `docs/VALIDACIONES/`) against the audio:
- The anonymization phase replaced common nouns across the whole text ("paciente" ~100 times, "médico", the `Investigador:` label itself, public institutions like FONASA/GES). It also nested placeholders (`[[ORGANIZACION_8]_4]`) and still missed a real person's name.
- The correction phase dropped ~3% of the content at chunk boundaries.
- Speaker labels inverted mid-interview.

The output was less usable than whisper's raw text. The user confirmed that researchers remove sensitive parts before recording, so automatic anonymization added little and cost much. The code is kept (not removed) so it can be re-enabled once its precision is fixed (nested replacement, common-noun filtering, structured output, glossary of non-PII terms). This does not weaken the local-only guarantee: nothing leaves the machine either way. But a default transcript is now **not** anonymized, and that must be stated wherever the pipeline is described.

---

## ADR-018

Per-Interview Keyword Glossary

Decision

Each interview can store a glossary of domain keywords (`interviews.keywords`, `PUT /interview/:id/keywords`).
- **Loading:** the user loads it from a plain `.txt` file (comma, semicolon or newline separated), parsed in the browser.
- **Whisper:** the glossary is the decoder's `initial_prompt` with `carry_initial_prompt`, truncated to 150 tokens. `n_max_text_ctx` grows by the prompt size so the 64-token rolling context is kept.
- **Ollama:** a `GlossarySanitizer` pass runs over whisper's segments before `transcript_final.txt` is written. The LLM only proposes `(original, termino)` pairs through Ollama structured outputs (`format` JSON schema, `num_predict=512`, new `ILLMClient::chatStructured`). The code validates each pair:
  1. The term must be in the glossary.
  2. The original must appear as a whole word in the block.
  3. The original must not itself be a glossary term.
  4. Normalized edit distance ≤ 50%.

  The code then replaces whole words only inside the 40-segment block where the pair was found. Changes are listed in `glossary_changes.txt`.

Reason

After ADR-017 the delivered transcript is plain whisper text, and its main remaining defect in the validated interview was domain vocabulary: SIGGES→SILYES, FONASA→Fonazo, CESFAM→exesfan, San Borja→Samborja, etapificación→tepificación.

**Two stages.** Whisper prompting fixes errors at the source. The sanitizer catches what whisper still misses, and whisper's prompt is capped at 224 tokens anyway.

**The LLM proposes and the code applies.** The opposite approach was rejected. Letting the LLM rewrite text lost ~3% of the content and broke the format in the same validation (ADR-017). A substitution table cannot drop or reorder content, keeps one line per segment, and every change is auditable.

**Structured outputs.** In the same run, free-text entity lists looped until `num_predict`.

**Per interview.** Reprocessing must not require reloading the file. A global list was deferred; the frontend offers the last interview's keywords instead.

**Known risks.** A glossary word that is also a common Spanish word (e.g. "gesto" mapped to GES) is replaced in the whole block if the model proposes it. Terms split across two whisper lines are not matched. Review `glossary_changes.txt`.

## ADR-019

In-App Transcript Reading View, PDF via Browser Print

Decision

The final transcript is read inside the app instead of only as a plain `.txt`.
- **Backend:** `GET /api/v1/interview/:id/transcript` returns the transcript as structured blocks (`TranscriptDocumentBuilder`, pure, no I/O): one block per speaker turn when the text has `Investigador:`/`Entrevistado:` labels, otherwise paragraphs of 4–9 whisper lines with the start second taken from `transcript_raw.json` (only when lines and segments match 1:1). The `[AVISO HERMES]` non-anonymized notice is returned apart, plus the summary if any. `410 TRANSCRIPTION_FILE_MISSING` when the DB has a result but the file is gone.
- **Frontend:** `/interviews/:id/transcript` renders a cover (interview record + review warning), summary, and the transcript in its own scroll panel, with search and highlighted anonymization markers (`[PERSONA_1]`).
- **PDF:** "Imprimir / Guardar PDF" uses the browser's print dialog with a `@media print` stylesheet (A4, white paper, cover on its own page, page numbers, no app chrome, scroll disabled).

Reason

A plain `.txt` with one whisper segment per line is very hard to read for researchers.

**No PDF/DOCX library in the backend.** Generating PDF or DOCX from C++ needs a new dependency (libharu, PoDoFo or similar). Browser printing gives a clean, paginated PDF with zero dependencies and works offline. DOCX (for NVivo/Atlas.ti) stays in Sprint 7 - Export.

**This is not Sprint 7 - Export.** Export stays blocked on anonymization reliability. Because a well-formatted document looks "finished", every rendering carries a review warning on the cover: plain whisper output is flagged as not anonymized, and anonymized output as possibly incomplete.

**Structuring in the backend.** The frontend is a pure API client; parsing speaker labels and pairing timestamps is backend logic.

---

## ADR-020

GPU Acceleration (Vulkan) and Fidelity-First Whisper Defaults

Decision

whisper.cpp is built with the vcpkg `vulkan` feature (`whisper-cpp[vulkan]` -> `ggml[vulkan]`). At model load (`WhisperTranscriber::ensureModelLoaded`) the GPU is used when ggml reports a GPU device (`WHISPER_USE_GPU=auto`, default; `1` forces the attempt, `0` forces CPU). If the GPU load fails, it falls back to CPU. On GPU:
- the default model is `ggml-large-v3` (`DEFAULT_WHISPER_MODEL_PATH`);
- the main pass uses beam search (beam 5);
- `flash_attn` is enabled (it stays off on CPU: that path crashed in this MinGW build).

On every run:
- `suppress_nst` drops non-speech tokens ("[Música]", "(risas)");
- `token_timestamps` gives per-word times;
- the Silero VAD is tuned: threshold 0.45, min speech 250 ms, min silence 400 ms, speech pad 200 ms, max speech 30 s;
- a short list of known subtitle hallucinations ("amara.org", "gracias por ver el video") is dropped. A segment is only dropped when it is the whole text of a segment of 3 s or less;
- a segment with an impossible text density (more than 35 characters per second plus a 10-character margin) is dropped only if it also repeats a nearby segment. A unique dense segment is real speech with a broken timestamp: on interview 2, ten 100 ms segments carried a real question;
- subtitle-credit hallucinations ("subtítulos por", "transcripción y subtítulos", "amara.org") are dropped wherever they appear, compared without accents;
- each 30 s window is conditioned only on the carried prompt, with no rolling context (`MAIN_PASS_MAX_TEXT_CTX = 0`). The prompt is the glossary plus a neutral, punctuated dialogue line (`STYLE_PROMPT`: "¿Y cómo lo hacen ustedes? Bueno, depende del caso, pero en general sí."). It contains no meta words such as "transcripción" or "entrevista";
- echoes of the prompt are dropped: a contiguous copy of at least 25 normalized characters of the prompt covering at least half of the segment. This is compared as continuous text because the style line uses common words;
- after the main pass, two repair passes re-transcribe ranges with no prompt and no context (beam 5):
  - stretches of 60 s or more without any punctuation, replaced only if the result is punctuated, loop-free and keeps at least 70% of the words;
  - gaps of 8 s or more without text, using `no_speech_thold = 0.8`. A silent gap returns nothing through the VAD.
- loop detection compares by containment for long texts (3 repeats) and by identity for short ones (4).

Segments now carry milliseconds and words (`TranscriptSegment{startMs,endMs,text,words}`) instead of truncated "HH:MM:SS" strings.

Reason

Transcription fidelity is one of the two requirements for Hermes to be minimally usable. The previous setup was `ggml-small` on CPU with greedy decoding. It was chosen only because medium and large are too slow on CPU, not on quality. The target machine has an AMD Radeon RX 9060 XT (16 GB).

**Why Vulkan.** It is the only GPU backend available with this toolchain. CUDA is excluded for `windows & staticcrt` in the ggml port. Vulkan works on AMD, NVIDIA and Intel.

**VRAM.** 16 GB fits large-v3 (~3-4 GB with caches) and Ollama 7B at the same time.

**Kept from the CPU setup.** The loop-repair retry stays. large-v3 is known to loop more than small.

**Validation on interview 6 (36 min, 2026-10-05).** large-v3 fixed most of small's word errors: "Ocriste"→"Existe", "pantologías"→"patologías", "octaculizan"→"obstaculizan", "GEES"→"GES", "fictéclica"→"ficha clínica". It also showed two failure modes:
- **Duplicated question.** It emitted three copies of the previous question in 100-360 ms segments (01:49). The density filter catches exactly those 3 of 379 segments.
- **Lost punctuation.** With the 64-token rolling context, one unpunctuated window at minute 18 left the rest of the interview without punctuation or capitals. small showed the same failure from minute 31. In whisper.cpp 1.8.6 with `carry_initial_prompt`, each window's prompt is the initial prompt plus the previous window's tokens, so the style of one window leaks into every following window.

Setting the rolling context to 0 and carrying a punctuated style sentence restored punctuation in every minute of interview 6, with the same word count (4988 vs 4984). Total time on the RX 9060 XT is about 13-15 min for the 36-min interview (transcription about 8, diarization about 4, glossary about 1).

**Style sentence reverted (2026-10-06).** On interviews 1, 2 and 10 (36-49 min, harder audio), the style sentence as the only context of every window caused two failures.
- **Echoed prompt.** On difficult stretches whisper copied it instead of transcribing: "Transcripción fiel de una entrevista, con puntuación…", 4 segments in a row in interview 2 (27 s).
- **Credit hallucination.** The word "Transcripción" primed "Transcripción y subtítulos por José Miguel Pinto…".
- **Dropped windows.** In the same kind of stretch, whisper.cpp dropped whole windows (`no_speech_prob > 0.6` and `avg_logprob < -1`; segments are only kept when `!is_no_speech`). That left 20-30 s gaps with speech: 45, 21 and 44 s lost, measured by crossing whisper segments with `diarization.json`.

Interview 6 did not show any of this because its audio is clean.

**Interview 2 (49 min) under the three configurations:**

| Configuration | Leaked text | Speech in gaps | Punctuated minutes | Words |
|---|---|---|---|---|
| Meta style sentence, context 0 | 5 | 21 s | 45/49 | 6319 |
| Glossary only, context 32, repairs | 0 | 0 s | 23/50 | 6751 |
| **Neutral style line, context 0, repairs + echo filter** | **0** | **0 s** | **48/49** | **6541** |

- **Without any style line,** this speaker's audio stays unpunctuated even when re-transcribed with no context. Only 2 of 7 unpunctuated stretches improved, so the repair alone is not enough.
- **With the neutral line,** the main pass was clean: no echo, no gap of 8 s or more, no unpunctuated stretch, no dropped segment.
- **The repairs and the echo filter stay as safety nets** for audio where whisper still copies or drops windows.

**Decoding thresholds.** whisper's temperature fallback and entropy/logprob thresholds were already at OpenAI's defaults, so they are not where the gain is.

**Word timestamps under VAD.** whisper.cpp 1.8.6 remaps segment times to the original audio under VAD, but not token times (`whisper_full_get_token_data` returns processed time). Word times are therefore projected linearly onto the remapped segment range: approximate, but enough to decide which side of a speaker change a word falls on (ADR-021).

**Costs:**
- `vulkan-loader` is a dynamic port, so `backend.exe` now needs `vulkan-1.dll`, which every modern GPU driver installs.
- The first vcpkg build compiles shaderc/glslang (~15 min).
- vcpkg's ggml build runs `glslc`, which is linked dynamically against MinGW's `libstdc++`. If another MinGW (Git's `/mingw64/bin`, msys64) comes first in `PATH`, `glslc` fails silently and ggml-vulkan ends up with empty shaders (undefined `*_data`/`*_len` symbols at link time). Put the project's MinGW first in `PATH` when building vcpkg dependencies.

Model comparison (A/B on real interviews): see `.ai/ROADMAP.md`.

---

## ADR-021

Acoustic Speaker Diarization with sherpa-onnx (Loaded at Runtime)

Decision

A new step `diarizando` runs after transcription, through a new interface `IDiarizer` (`backend/diarization/`). It identifies who speaks when from the audio itself. `SherpaOnnxDiarizer` uses sherpa-onnx v1.13.8 (official `win-x64-shared-MD-Release` build) with:
- pyannote segmentation-3.0 (ONNX, MIT);
- 3D-Speaker CAM++ embeddings (`3dspeaker_speech_campplus_sv_zh_en_16k-common_advanced.onnx`);
- fast clustering **by distance threshold (0.7)**, not a fixed number of clusters (`DIARIZATION_NUM_SPEAKERS=0` by default; `N > 0` forces N).

**Runtime loading.** The C API DLL (`sherpa-onnx-c-api.dll` + `onnxruntime.dll`, in `./sherpa-onnx`) is loaded with `LoadLibraryExW` + `GetProcAddress`, not linked. Only the C header is vendored (`backend/third_party/sherpa-onnx/`, Apache-2.0, same tag as the DLL).

**Missing files.** If the DLL or a model is missing, `isAvailable()` is false. The pipeline logs how to install them and continues without speakers. The step never fails the job.

**Aligning words with speakers.** `SpeakerAssigner` (pure) matches whisper's segments to the speaker turns:
- a segment goes to the speaker covering ≥70% of it, unless the other speaker covers 2 s or more (whisper segments reach 30 s, and a short question inside a long answer is a real turn);
- otherwise it is split word by word using each word's midpoint, and runs of fewer than 2 words are absorbed into the neighbour;
- each split point is moved to the nearest sentence end (`.`, `?`, `!`) within 4 words. Without a nearby sentence end, a split 2 words or less from the segment edge is dropped. This absorbs the 1-4-word error of projected word times and diarization boundaries, which otherwise produced cuts such as "Apague la cámara por | subir la señal".
- The raw speaker turns are written to `diarization.json` (times and cluster only, no text) for traceability.

**Relevant clusters.** A cluster counts as a person when it has at least 3% of the total speech and at least 3 turns. Turns of the other (small) clusters are relabelled to the relevant cluster of the nearest turn in time, before aligning with the words. Small clusters are noise, laughter or backchannels.

**Role decision.**
- The interviewer is the relevant cluster with the highest share of questions (`¿`/`?`). On a tie (< 0.05) with the largest other cluster, it is the one who talks less; then the one who speaks first.
- Every other relevant cluster goes to the role whose question share it resembles. In practice this is the subject, whose voice is often split into two or more clusters.
- The interviewer is labelled "Investigador". The other role is labelled with the interview's own `subject_type` (e.g. "Monitor GES"), as requested by the user.
- With fewer than 2 relevant clusters, the transcript is delivered without speakers rather than guessing.

**Why a threshold instead of 2 clusters (2026-10-05, interview 7, 49 min).**
- **Forcing 2 clusters on the whole recording** returned 2481 s vs 31 s. The subject's voice ended up in one group together with the interviewer's, and only stray fragments formed the other group, so the transcript came out without speakers.
- **The first 10 minutes of the same audio** separated correctly with 2 clusters, so the failure is global clustering over a long recording, not the voices.
- **With threshold 0.7**, the interviewer is one cluster (37 of 48 segments are questions) and the subject is split into two (02 and 05, present throughout).
- **Interview 6 keeps working** under the same rule (interviewer question share 0.59 vs 0.05).
- **Interview 9** (a 2-minute puppet sketch with acted voices and music) yields a single voice under every configuration and is delivered without speakers.
- **Model choice.** wespeaker with 2 clusters failed the same way as campplus.

Reason

Speaker separation is the second requirement for minimum usability. Until now the only speaker labels came from the opt-in LLM pass, which guessed them from text and inverted speakers mid-interview (ADR-017). There is no audio signal in text.

**Alternatives considered:**
- *whisper.cpp tinydiarize:* rejected, it only works with an English `small.en-tdrz` model.
- *onnxruntime from vcpkg built with MinGW:* rejected, not a supported configuration.
- *pyannote in Python:* rejected, it would add a Python runtime to a C++ monolith.

**Why runtime loading.** Loading the C API at runtime avoids mixing the MSVC-built release into the MinGW link. The backend also keeps starting without it. No C++ types cross the boundary: every buffer the DLL returns is freed with its own `Destroy*` function, because the two CRTs do not share a heap.

**Spike (2026-10-05, interview 6, 36 min, CPU):**
- campplus and wespeaker-resnet34 agreed on 367 of 369 segments; campplus was ~30% faster (220 s vs 325 s);
- the questions fell on one cluster and the answers on the other.

**Privacy.** Models and DLLs are manual downloads. Nothing touches the network at runtime.

**Known limits.** Overlapping speech, very short backchannels ("ya", "mhm") and interviews with more than two people. The reading-view editor (ADR-022) is the correction path, including a one-click swap of the two roles.

---

## ADR-022

Structured Transcript as Source of Truth, with Manual Editing

Decision

**Storage.** The delivered transcript is a JSON document per interview:
- `storage/interviews/<id>/transcript_segments.json` holds `{version, speaker_source, notice, turns:[{start_ms,end_ms,speaker,text}]}`, behind `ITranscriptStore` / `FileTranscriptStore`, written atomically (tmp + rename);
- `speaker` stores the **role key** (`interviewer` / `subject`), not a name. Display labels are resolved at read time ("Investigador" and `interviews.interview_subject_type`), so renaming the subject never requires reprocessing.

**Derived outputs.** `transcript_final.txt` is rendered from the pipeline JSON (`renderPlainText`). The `.txt` download is rendered live from the current version, so it reflects edits and current labels.

**Reading view.** `GET /interview/:id/transcript` returns blocks with role keys, start/end seconds, `speakers` (key→label), `speaker_source` and `edited`.

**Editing:**
- an "Editar" mode in the reading view edits text and speaker per turn, plus split / merge / delete and "swap both speakers";
- edits are saved with `PUT /interview/:id/transcript` to `transcript_edited.json`. The pipeline file is never overwritten;
- `DELETE /interview/:id/transcript/edits` restores the original;
- edits are rejected while a job is active (409). Reprocessing deletes them, and the UI warns first (`transcript_edited` in the interview detail).

**LLM correction.** The opt-in correction (`TranscriptEnhancer`) no longer labels speakers:
- it receives indexed turns (`[n] (Hablante) texto`) and must return the same indexes;
- a missing, garbled or implausibly changed line (length ratio outside 0.6-1.6) keeps its original text;
- anonymization substitutes per turn, and entities are extracted from text without labels.

Interviews processed before this change still render through the legacy `.txt` parser (`buildLegacy`), and they cannot be edited until reprocessed.

Supersedes the speaker-labeling part of ADR-017 and the label parsing of ADR-019.

Reason

A diarized transcript needs a format that keeps the speaker and the times of every turn. Plain "Label: text" lines lose the times and tie the file to fixed label names. They are also what made the old reader drop timestamps entirely: `readSegmentStarts` parsed `"start":"00:00:02"` as a number and always threw.

**Editing.** Even good diarization makes mistakes, and researchers need to fix both the attribution and whisper's text. The user asked for an in-place edit mode with persisted changes.

**Two files.** Keeping the pipeline output and the edits separate keeps traceability and allows restoring the original.

**Why the LLM stopped labeling speakers.** Labeling from text was the source of the inversions and of the ~3% content loss at chunk boundaries (ADR-017). Per-index correction with fallback to the original cannot lose turns.

---

## ADR-023

Synchronized Audio Playback in the Reading View

Decision

The reading view plays the interview audio and highlights the transcript block being heard, in both reading and edit mode.

**Backend.** New `GET /api/v1/interview/:id/audio` serves the normalized `storage/interviews/<id>/audio.wav` (the exact audio the timestamps refer to).
- With a `Range` header (`bytes=a-b`, `bytes=a-`, `bytes=-n`), it answers `206 Partial Content` with `Content-Range`, capped at 2 MB per response. The browser keeps requesting the following ranges.
- Without `Range`, it answers `200` with the whole file streamed by Crow (`set_static_file_info_unsafe`, read in chunks, never fully in memory).
- `416` for a range past the end; `404 AUDIO_NOT_AVAILABLE` when the interview was never processed.
- `InterviewService::readAudio` resolves the path, validates the interview and reads the bytes. The controller only parses the `Range` header and builds the response.

**Frontend.** A native `<audio preload="metadata">` player sits fixed below the transcript panel, with:
- play/pause, back/forward 5 s and speed (0.75×-1.5×);
- shortcuts Alt+K (play/pause) and Alt+J / Alt+L (5 s back/forward). They work while typing in the editor, and avoid Alt+←/→, which is "back/forward" in the browser and would leave the page mid-edit;
- a "follow audio" toggle that scrolls the active block into view;
- clicking a block's timestamp seeks the audio there.

**Granularity.** Per block, using the `start`/`end` already returned by `GET /interview/:id/transcript`. Word-level highlighting is deferred: word times are approximate under VAD (ADR-020) and would require exposing words in the structured transcript.

**Edited transcripts.** Blocks without `start` (created by splitting in the editor) inherit the time of the previous block, and both halves are highlighted together (`activeBlockRange` in `frontend/src/audioTimeline.ts`). In the editor, "follow audio" does not scroll while a text box has focus, so the text under the cursor doesn't move.

Reason

Correcting a transcript (ADR-022) requires listening to the audio: checking dubious words (e.g. "CESFAM"), fixing speaker attribution and resolving gaps. Until now the user had to open the audio in an external player and look for the timestamp by hand.

- **No new dependency:** the browser's native `<audio>` element plus HTTP `Range` is enough.
- **Range by hand:** Crow 1.3.3 serves static files only as `200` with the whole body (`http_response.h`, `set_static_file_info`). Without `206`, the browser cannot seek in a ~115 MB/hour WAV without downloading it.
- **Why `audio.wav` and not the upload:** the original may not exist (a video is replaced by its extracted audio, ADR-016), and the WAV is the time reference of the timestamps.
- **Why 2 MB per response:** a `bytes=0-` request (what browsers send first) would otherwise load the whole file into memory. Browsers handle short `206` responses and continue.
- **Privacy:** everything stays local. The backend listens on `0.0.0.0` without authentication, so the audio becomes reachable from the local network just like transcripts already are (README § Limitaciones conocidas).
