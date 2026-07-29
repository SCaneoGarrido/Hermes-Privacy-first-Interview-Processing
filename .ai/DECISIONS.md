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