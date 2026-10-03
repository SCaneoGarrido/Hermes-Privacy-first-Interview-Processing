# Roadmap

13 sprints, Sprint 0 to Sprint 12. Each sprint must leave the project deployable.

## Current release: v0.1.0 (pre-release / early preview)

Version prepared 2026-08-15 (README.md, CHANGELOG.md, CMake project version); not yet git-tagged or pushed as of this writing. End-to-end pipeline (upload -> transcribe -> correct/anonymize -> download) works and was verified against real interviews, but the project does not yet meet the scope originally defined for v1.0 -- see README.md ("Qué falta para v1.0" and "Limitaciones conocidas") for the user-facing version of this list. Not recommended for processing real sensitive data without human review of the anonymization output.

## Sprint 0 - Foundation

Status: Done

Environment, CMake/vcpkg, Crow, MySQL, logging, `/health`.

## Sprint 1 - Core API

Status: In progress

Services/Repositories and API versioning (`/api/v1`) resolved (ADR-012, ADR-013). Configuration endpoint and formal DTOs still pending.

## Sprint 2 - Persistence

Status: Done

Full CRUD (including `DELETE`), `IInterviewRepository`/`InterviewService` (ADR-012).

2026-10-02: `DatabaseManager` reconnects automatically. It used a single connection opened at startup and never reopened it, so a MySQL restart (`docker compose restart`) left the backend answering every query with "Server has gone away" until it was restarted. Now `ensureConnected()` runs `mysql_ping` under the DB mutex before each query and reopens the connection with the stored parameters if it was lost. Verified by the user. Still pending: `executeQuery` returns an empty result on error, so `GET /interviews` answers `[]` with `success: true` instead of an error.

## Sprint 3 - File Upload

Status: In progress

Upload with byte-signature validation done. Upload progress reporting still pending.

2026-10-02: `.mp4` video accepted as input (audio extracted by the existing normalizer, original video deleted after extraction) and a 1 GiB upload cap (413) -- ADR-016. Verified 2026-10-02 with a real 143 MB .mp4 (49 min): audio extracted, video deleted, `interviews_audio` repointed to the .wav.

2026-10-02: **audio replacement and no orphan uploads.** Uploading a second file to the same interview failed with `Duplicate entry` (UNIQUE `interview_id`), and the file was written to disk *before* the DB insert, so every failed upload stayed in `./uploads` with no record (12 of 13 files, ~535 MB, deleted with the user's approval). Now:
- `InterviewService::canAttachAudio` runs before writing the file: `404` if the interview does not exist, `409 INTERVIEW_BUSY` if it is processing or has a queued job.
- If the interview already had audio, the row is updated, the old file is deleted, and the previous result (`interview_results` row + `storage/interviews/<id>`) is discarded because it no longer matches the audio. The frontend asks for confirmation.
- Any rejected upload is deleted from disk.
- Deleting an interview now also deletes `storage/interviews/<id>` (before, only the audio file was removed).

## Sprint 4 - Background Processing

Status: Done

Job queue (in-memory, thread-safe), worker pool (`WORKER_POOL_SIZE`, default 1), `interview_jobs` state machine (pending/running/completed/failed), dedupe (409 on already-queued), crash recovery (`reclaimStuckJobs`), mutex on `DatabaseManager` closing the race condition that existed since Sprint 0.

## Sprint 5 - Whisper Integration

Status: Done

`audio/` (FFmpeg static via vcpkg, ADR-014) + `transcription/` (whisper.cpp, lazy model load, mutex on `whisper_context`). Model: ggml-small, language forced to "es" by default (`WHISPER_LANGUAGE`). Verified end-to-end against a real ~70min interview (1804 segments).

## Sprint 6 - Ollama Integration

Status: Done, with known quality gaps

`llm/` (cpp-httplib, ADR-015; `OllamaClient`, `TranscriptEnhancer`). Three phases over whisper's raw segments: correction + speaker labeling (chunked), anonymization (two-pass entity table for consistency), summary (map-reduce, opt-in via `include_summary`, off by default). Verified at real scale. Known issues, documented in `hermes-vault-knowledge/08 AI/Ollama Integration Strategy.md`: entity-extraction recall is incomplete (some public figures not anonymized); speaker attribution is not deterministic between runs of the same audio; the model sometimes emits label variants outside the two requested; whisper.cpp can hallucinate text in another language on unclear audio. None of this blocks using the transcript as plain text; all of it blocks trusting the output as a reliable diarization/anonymization without human review.

**Open backlog (2026-07-29)**: prioritized fix plan in the same vault note. Items 1 (label normalization) and 2 (`temperature`/`seed` for determinism) implemented 2026-08-15. Still pending: language/alphabet validation, entity recall (the most important -- blocks Sprint 7). GPU acceleration (Vulkan for whisper.cpp, GPU detection + advisory logging for Ollama) researched and planned in `hermes-vault-knowledge/08 AI/GPU Acceleration Strategy.md`, not yet implemented.

**Unrelated bug found and fixed 2026-08-15 while testing the above**: downloaded transcripts showed mojibake accents (e.g. `Â¿QuÃ©` instead of `¿Qué`) when opened in some Windows text editors. Root cause was not the pipeline -- `transcript_final.txt` on disk and the HTTP response were both confirmed correct UTF-8 -- but the absence of a BOM, which makes editors without reliable UTF-8 auto-detection fall back to the system ANSI codepage. Fixed by prepending a UTF-8 BOM in `buildDownloadResponse` (`backend/api/controllers/interviewController.cpp`), download-response only, not in the stored file (so nothing that re-reads `transcript_final.txt` internally, e.g. future Export, is affected). Also found during the same session: a label variant (`Investervistado:`) at edit-distance 6 from both canonical labels (tied) -- deliberately left unfixed rather than guessing the speaker, documented as an open finding in `hermes-vault-knowledge/08 AI/Ollama Integration Strategy.md`.

**Transcription quality pass (2026-10-02)**, from the deep manual validation of interview 13 (`docs/VALIDACIONES/`):
- Root causes:
  - whisper repetition loops (raw JSON: x26 and x114 repeated segments, ~7 min lost).
  - Ollama silent context shift at its 4096 default.
  - A runaway generation that hit the 300 s timeout and, being all-or-nothing, discarded the finished correction phase.
- Phase 1 implemented:
  - Bounded whisper text context.
  - Loop detection + re-transcription.
  - Optional Silero VAD.
  - `num_ctx`/`num_predict`.
  - Per-phase degradation with a non-anonymized notice.
- Verified on the same recording (`interview_id=1`): 0 repeated segments, full coverage to 49:05.
- The same run exposed anonymization over-substitution (common nouns, nested placeholders, a missed real name) and ~3% content loss at correction chunk boundaries.
- **Correction + anonymization made opt-in (`enhance_transcript`, default off) -- ADR-017.** Default output is plain whisper text; the summary is independent and always a separate file.
- Per-interview keyword glossary implemented 2026-10-02 (ADR-018): whisper `initial_prompt` + `GlossarySanitizer` (LLM proposes pairs via structured outputs, code validates and replaces). Pending real-audio verification against `docs/VALIDACIONES/`.
- Next:
  - Fix anonymization precision before recommending `enhance_transcript`.
  - Model/beam comparison.

## Sprint 7 - Export

Status: Not started

Blocked on Sprint 6's anonymization reliability before it's safe to export by default.

2026-10-02: the in-app reading view with "Imprimir / Guardar PDF" from the browser (ADR-019) is **not** this sprint -- it renders the transcript for reading and review, with a review warning on the cover. Formal export (DOCX for NVivo/Atlas.ti, JSON) remains here and remains blocked.

## Sprint 8 - Frontend

Status: In progress (ahead of order)

React + TypeScript + Vite. Interview list/detail, upload, process (with optional summary flag), progress polling with elapsed time and current step, transcript/summary download.

2026-10-02:
- **Visual redesign** from two mockups (`docs/img/mockups/`): classical Greek identity (Attic black, terracotta, ivory, Aegean blue, old gold; epigraphic titles with system serif fonts; meander dividers; own SVG icons and logo). No new dependencies and nothing loaded from the network. Mockup elements with no API data behind them were dropped (fake metrics, cancel button, CPU console, "cryptographic" claims).
- List: stat cards, client-side search/sort/status filters, cards with one primary action per status, custom delete dialog, auto-refresh while something is processing.
- Detail: per-status layout; "threshold sequence" stepper (I–VI) for processing steps; drag-and-drop upload with client validation; replace-audio confirmation.
- The API does not return which options a running job was launched with, so the browser remembers the options of jobs it launched; unknown optional steps are shown as "Si se pidió".
- **Reading view** `/interviews/:id/transcript` (ADR-019): cover with record and review warning, summary, transcript in its own scroll panel, search with jump to first match, highlighted `[PERSONA_1]` markers, print stylesheet for PDF.
- Light mode is defined but was not visually reviewed.

## Sprint 9 - Configuration

Status: Not started

## Sprint 10 - Testing

Status: Not started

No Catch2 tests yet, only manual verification.

## Sprint 11 - Documentation

Status: Underestimated

`.ai/`, `hermes-vault-knowledge/`, and `docs/API_REQUIREMENTS.md` already exist and are actively maintained. Formal install guide still missing.

## Sprint 12 - Release

Status: In progress (pre-release only)

v0.1.0 pre-release prepared 2026-08-15 (README, CHANGELOG, CMake version), not yet git-tagged. The v1.0 release this sprint defines is not started.
