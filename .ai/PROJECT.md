# Hermes

Privacy-first Interview Processing

---

# Project Overview

Hermes is an open-source platform designed for local interview processing.

The system focuses on qualitative research where interviews may contain highly sensitive information, including patient data, healthcare professionals, or personal identifiers.

For this reason, Hermes follows a strict Local First philosophy.

No cloud services are required.

No external APIs are required.

All AI models execute locally.

---

# Mission

Build a privacy-first platform that allows researchers to:

- Upload interviews
- Transcribe audio locally
- Anonymize sensitive information
- Generate summaries
- Export documents

without exposing confidential information.

---

# Core Principles

These principles are immutable.

1. Privacy First

Sensitive information must never leave the local machine unless explicitly configured by the user.

2. Local First

The platform must work without Internet access.

3. API First

All business logic must be exposed through REST APIs.

The frontend is only a client.

4. Modular Monolith

Hermes is NOT a microservice architecture.

The project consists of one executable with multiple independent modules.

5. Clean Architecture

Frameworks are implementation details.

Business logic must never depend on Crow, SQLite, Whisper or Ollama.

6. Open Source

The code should remain understandable and accessible.

7. Maintainability over Cleverness

Readable code is preferred over complex optimizations.

---

# Technical Stack

Language

C++20

Frontend

React + TypeScript

REST Framework

Crow

Database

SQLite

Speech Recognition

whisper.cpp

LLM

Ollama

Default Model

Qwen

Logging

spdlog

Build System

CMake

Package Manager

vcpkg

Testing

Catch2

Serialization

nlohmann/json

---

# Repository Philosophy

Every external dependency should be isolated behind an interface.

Examples

ITranscriber

↓

WhisperTranscriber

ILLMClient

↓

OllamaClient

Repositories should hide SQLite.

Controllers should never contain business logic.

---

# Out of Scope

The following features must NOT be added unless explicitly approved.

Authentication

Cloud processing

Azure

OpenAI API

AWS

Google Cloud

Microservices

Kubernetes

Distributed processing

Message brokers

Complex infrastructure

---

# Development Philosophy

The project should evolve in small iterations.

Each Sprint must produce a working application.

No feature should break previous functionality.

Every Sprint must leave the project in a deployable state.

---

# Long Term Goal

Hermes should become an open-source platform that any researcher can install and use locally in less than 10 minutes.