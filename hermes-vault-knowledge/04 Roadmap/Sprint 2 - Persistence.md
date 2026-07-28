---
title: Sprint 2 - Persistence
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 1 - Core API", "Sprint 3 - File Upload", "ADR-008 - MySQL como Base de Datos", "ADR-009 - libmariadb como Cliente MySQL", "MariaDB Connector-C (libmariadb)", "Filosofia de Repositorios"]
---

# Summary

Persistencia de entrevistas mediante MySQL: CRUD, gestión de archivos y metadata.

# Explanation

**Objetivo:** persistencia de entrevistas.

**Entregables (2026-07-28):** MySQL vía [[ADR-008 - MySQL como Base de Datos]] y [[ADR-009 - libmariadb como Cliente MySQL]], gestión de archivos, metadata, directorios de almacenamiento — completos. CRUD de entrevistas — parcial (ver funcionalidades).

**Funcionalidades:**
- ✅ Crear entrevista (`POST /interview`, devuelve el `id` insertado)
- ✅ Consultar entrevista (`GET /interview/:id`, incluye audio y resultado asociados)
- ✅ Listado (`GET /interviews`)
- ❌ Eliminar entrevista — no implementado

`DatabaseManager` (`backend/api/shared/database/`) expone `executePrepared` (INSERT/UPDATE, con `SqlParam = std::variant<int, long long, std::string>` para parámetros tipados) y `executeQuery` (SELECT, mapea columnas de vuelta a `SqlParam` respetando el tipo real de MySQL — ver [[MariaDB Connector-C (libmariadb)]] para el detalle de por qué las columnas `DATETIME`/`TIMESTAMP` necesitan bindearse como texto).

# Why it matters

Es el primer sprint que ejercita [[ADR-008 - MySQL como Base de Datos]] en la práctica.

# Best Practices

- Definir el repositorio de entrevistas como interfaz antes de implementarlo sobre MySQL (ver desviación abajo).

# Common Mistakes

- Acoplar la estructura de la tabla MySQL directamente a los DTOs expuestos por la API.

# Hermes Usage

**Desviación conocida de [[Filosofia de Repositorios]] (2026-07-28)**: `InterviewController` y `FileController` llaman directamente a `DatabaseManager::getInstance()` — no existe todavía una interfaz `IInterviewRepository` (o similar) que oculte MySQL detrás de una abstracción, como exige la filosofía de repositorios del proyecto. Es deuda técnica explícita, no un cambio de principio: se aceptó para avanzar rápido con el flujo end-to-end (crear → subir audio → procesar) antes de introducir la capa de abstracción.

# Related Notes

- [[Sprint 1 - Core API]]
- [[Sprint 3 - File Upload]]
- [[ADR-008 - MySQL como Base de Datos]]
- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[Filosofia de Repositorios]]

# References

- README.md (Priority 2)
- docs/API_REQUIREMENTS.md (Priority 1 — contrato real de los endpoints de entrevistas)
