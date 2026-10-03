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
