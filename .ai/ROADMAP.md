# Roadmap

13 sprints, Sprint 0 to Sprint 12. Each sprint must leave the project deployable.

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

**Open backlog (2026-07-29, not yet implemented)**: prioritized fix plan in the same vault note (label normalization, `temperature`/`seed` for determinism, language/alphabet validation, entity recall -- in that order). GPU acceleration (Vulkan for whisper.cpp, GPU detection + advisory logging for Ollama) researched and planned in `hermes-vault-knowledge/08 AI/GPU Acceleration Strategy.md`, also not yet implemented.

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
