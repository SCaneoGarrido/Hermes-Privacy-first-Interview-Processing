# Hermes

> Privacy-first Interview Processing

Hermes es una plataforma open source diseñada para la transcripción local de entrevistas de investigación, anonimización de información sensible y apoyo al análisis cualitativo, priorizando la privacidad de los datos y evitando la dependencia de servicios cloud.

---

# Objetivos

## Objetivo principal

Desarrollar una plataforma local que permita a investigadores procesar entrevistas de manera segura mediante inteligencia artificial ejecutándose completamente en infraestructura propia.

## Objetivos secundarios

- Aprender y aplicar C++ moderno.
- Diseñar una arquitectura mantenible y escalable.
- Crear un proyecto open source reutilizable.
- Permitir que futuros estudiantes puedan instalar Hermes sin conocimientos avanzados de programación.

---

# Filosofía del proyecto

Hermes seguirá los siguientes principios:

- Privacy First
- Local First
- Open Source
- Modular Monolith
- API First
- Clean Architecture
- Framework Agnostic
- Testable
- Extensible

---

# Arquitectura

Se utilizará una arquitectura:

Monolito Modular

con separación por capas:

```
API

↓

Application

↓

Domain

↓

Infrastructure
```

Cada módulo tendrá una única responsabilidad.

---

# Roadmap

---

# Sprint 0 — Foundation

## Objetivo

Preparar completamente el entorno de desarrollo.

## Entregables

- Repositorio Git
- Configuración CMake
- Configuración vcpkg
- Primer proyecto compilando
- Configuración de Crow
- Configuración SQLite
- Configuración Logging
- Primer endpoint REST
- Documentación inicial

## Endpoints

GET /api/v1/health

GET /api/v1/info

---

# Sprint 1 — Core API

## Objetivo

Construir el núcleo del backend.

## Entregables

- Estructura modular
- Controllers
- Services
- Repositories
- DTOs
- Configuración
- Manejo de errores
- Logging
- Versionado API

## Endpoints

Interview

Configuration

Health

---

# Sprint 2 — Persistence

## Objetivo

Persistencia de entrevistas.

## Entregables

- SQLite
- CRUD entrevistas
- Gestión de archivos
- Metadata
- Directorios de almacenamiento

## Funcionalidades

Crear entrevista

Eliminar entrevista

Consultar entrevista

Listado

---

# Sprint 3 — File Upload

## Objetivo

Carga de audios.

## Entregables

- Upload Multipart
- Validación
- Organización de archivos
- Identificadores únicos
- Validaciones

---

# Sprint 4 — Background Processing

## Objetivo

Procesamiento asíncrono.

## Entregables

- Job Queue
- Worker Threads
- Estados
- Progress Tracking

Estados

Pending

Running

Completed

Failed

---

# Sprint 5 — Whisper Integration

## Objetivo

Transcripción local.

## Entregables

Integración whisper.cpp

Generación TXT

Generación JSON

Detección idioma

Configuración modelo

---

# Sprint 6 — Ollama Integration

## Objetivo

Integración IA local.

## Entregables

Cliente HTTP

Corrección ortográfica

Puntuación

Resúmenes

Anonimización

---

# Sprint 7 — Export

## Objetivo

Exportación.

## Entregables

TXT

DOCX

PDF

JSON

---

# Sprint 8 — Frontend

## Objetivo

Interfaz web.

## Entregables

React

Carga entrevistas

Listado

Detalle

Progreso

Descargas

Configuración

---

# Sprint 9 — Configuration

## Objetivo

Configuración completa.

## Entregables

Configuración modelos

Configuración almacenamiento

Configuración idioma

Configuración puertos

Configuración LLM

---

# Sprint 10 — Testing

## Objetivo

Calidad.

## Entregables

Unit Tests

Integration Tests

Stress Tests

Logging

Benchmarks

---

# Sprint 11 — Documentation

## Objetivo

Documentación completa.

## Entregables

README

Installation Guide

Developer Guide

Architecture

API Documentation

Contribution Guide

---

# Sprint 12 — Release

## Objetivo

Primera versión estable.

## Entregables

Versión 1.0

Release Notes

Instalador Windows

Documentación

Repositorio público

---

# Estructura del proyecto

```
Hermes/

backend/

frontend/

docs/

scripts/

tests/

storage/

README.md

LICENSE

CHANGELOG.md
```

---

# Convenciones

## Idioma

Código

Inglés

Documentación

Inglés

Manual de usuario

Español (inicialmente)

---

## Convenciones Git

main

develop

feature/*

fix/*

release/*

hotfix/*

---

## Versionado

Semantic Versioning

MAJOR.MINOR.PATCH

Ejemplo

1.0.0

---

# Objetivos de la versión 1.0

- API REST funcional
- Procesamiento local
- Whisper.cpp
- Ollama
- Exportación DOCX
- React
- SQLite
- Instalador Windows
- Documentación completa

---

# Fuera del alcance (v1.0)

No se implementará:

- Autenticación
- Usuarios
- Roles
- Multiempresa
- Procesamiento distribuido
- Kubernetes
- Docker obligatorio
- PostgreSQL
- Microservicios
- Integraciones cloud
- Sincronización online

Estos elementos podrán evaluarse para futuras versiones.

---

# Visión

Hermes busca convertirse en una herramienta open source para investigadores que necesiten procesar entrevistas de manera privada, segura y completamente local, utilizando inteligencia artificial sin depender de servicios externos.