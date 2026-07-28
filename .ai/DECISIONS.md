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