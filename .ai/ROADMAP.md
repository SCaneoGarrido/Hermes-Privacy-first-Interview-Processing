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

## Sprint 3 - File Upload

Status: In progress

Upload with byte-signature validation done. Upload progress reporting still pending.

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

## Sprint 7 - Export

Status: Not started

Blocked on Sprint 6's anonymization reliability before it's safe to export by default.

## Sprint 8 - Frontend

Status: In progress (ahead of order)

React + TypeScript + Vite. Interview list/detail, upload, process (with optional summary flag), progress polling with elapsed time and current step, transcript/summary download.

## Sprint 9 - Configuration

Status: Not started

## Sprint 10 - Testing

Status: Not started

No Catch2 tests yet, only manual verification.

## Sprint 11 - Documentation

Status: Underestimated

`.ai/`, `hermes-vault-knowledge/`, and `docs/API_REQUIREMENTS.md` already exist and are actively maintained. Formal install guide still missing.

## Sprint 12 - Release

Status: Not started
