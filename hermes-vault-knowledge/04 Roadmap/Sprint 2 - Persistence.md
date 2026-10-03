---
title: Sprint 2 - Persistence
aliases: []
tags: [roadmap, sprint, hermes]
status: done
created: 2026-07-24
updated: 2026-10-02
source: README.md
related: ["Sprint 1 - Core API", "Sprint 3 - File Upload", "ADR-008 - MySQL como Base de Datos", "ADR-009 - libmariadb como Cliente MySQL", "MariaDB Connector-C (libmariadb)", "Filosofia de Repositorios"]
---

# Summary

Persistencia de entrevistas mediante MySQL: CRUD, gestión de archivos y metadata.

# Explanation

**Objetivo:** persistencia de entrevistas.

**Entregables (2026-07-28):** MySQL vía [[ADR-008 - MySQL como Base de Datos]] y [[ADR-009 - libmariadb como Cliente MySQL]], gestión de archivos, metadata, directorios de almacenamiento, CRUD de entrevistas — completos.

**Funcionalidades:**
- ✅ Crear entrevista (`POST /interview`, devuelve el `id` insertado)
- ✅ Consultar entrevista (`GET /interview/:id`, incluye audio y resultado asociados)
- ✅ Listado (`GET /interviews`)
- ✅ Eliminar entrevista (`DELETE /interview/:id`) — borra la fila, la cascada de MySQL se encarga de `interviews_audio`/`interview_results`, y además borra el archivo de audio real en `./uploads` (Privacy First: sin esto quedaría huérfano en disco)

`DatabaseManager` (`backend/api/shared/database/`) expone `executePrepared` (INSERT/UPDATE, con `SqlParam = std::variant<int, long long, std::string>` para parámetros tipados) y `executeQuery` (SELECT, mapea columnas de vuelta a `SqlParam` respetando el tipo real de MySQL — ver [[MariaDB Connector-C (libmariadb)]] para el detalle de por qué las columnas `DATETIME`/`TIMESTAMP` necesitan bindearse como texto). Ya no se llama directo desde los controllers (ver abajo).

**Actualización 2026-10-02 — reconexión a MySQL:** `DatabaseManager` abría una sola conexión al iniciar y nunca la reabría. Tras un reinicio de MySQL (`docker compose restart`) el backend respondía todo con "Server has gone away" hasta reiniciarlo (además había dos `backend.exe` corriendo a la vez, uno con la conexión muerta). Ahora `ensureConnected()` hace `mysql_ping` bajo el mutex antes de cada consulta y reabre la conexión con los parámetros guardados (solo en memoria, nunca en el log). Verificado por el usuario. **Pendiente:** `executeQuery` devuelve un resultado vacío ante un error, así que `GET /interviews` responde `[]` con `success: true` en vez de un error.

# Why it matters

Es el primer sprint que ejercita [[ADR-008 - MySQL como Base de Datos]] en la práctica.

# Best Practices

- Definir el repositorio de entrevistas como interfaz antes de implementarlo sobre MySQL — resuelto vía `IInterviewRepository`/`MySqlInterviewRepository` (ver abajo).

# Common Mistakes

- Acoplar la estructura de la tabla MySQL directamente a los DTOs expuestos por la API.

# Hermes Usage

**Desviación de [[Filosofia de Repositorios]] resuelta (2026-07-28)**: `InterviewController` y `FileController` llamaban directamente a `DatabaseManager::getInstance()`. Se cerró introduciendo `IInterviewRepository` (implementada por `MySqlInterviewRepository`) y una capa `InterviewService` fina entre los controllers y el repositorio — ver ADR-012 en `.ai/DECISIONS.md`. Se hizo antes de [[Sprint 4 - Background Processing]] a propósito: con solo 2 controllers acoplados, retrofitear la abstracción era más barato ahora que después de sumar la cola de trabajos.

# Related Notes

- [[Sprint 1 - Core API]]
- [[Sprint 3 - File Upload]]
- [[Sprint 4 - Background Processing]]
- [[ADR-008 - MySQL como Base de Datos]]
- [[ADR-009 - libmariadb como Cliente MySQL]]
- [[Filosofia de Repositorios]]

# References

- README.md (Priority 2)
- docs/API_REQUIREMENTS.md (Priority 1 — contrato real de los endpoints de entrevistas)
- .ai/DECISIONS.md (Priority 1 — ADR-012)
