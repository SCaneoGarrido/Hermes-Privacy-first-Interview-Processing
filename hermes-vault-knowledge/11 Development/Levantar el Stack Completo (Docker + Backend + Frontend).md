---
title: Levantar el Stack Completo (Docker + Backend + Frontend)
aliases: ["Correr Hermes End-to-End", "Docker Compose + Backend + Frontend"]
tags: [development, hermes, docker, mysql, frontend, build]
status: stable
created: 2026-07-28
updated: 2026-07-28
source: docker-compose.yml, backend/main.cpp, frontend/vite.config.ts
related: ["Compilar y Ejecutar el Backend con CMake", "ADR-010 - Docker Compose para MySQL Local", "MariaDB Connector-C (libmariadb)", "Sprint 8 - Frontend"]
---

# Summary

Flujo verificado para levantar Hermes completo en desarrollo: MySQL en Docker, el backend de C++, y el frontend de React — en ese orden, con las variables de entorno y gotchas de Windows encontrados en la práctica.

# Explanation

**1. Base de datos (MySQL en Docker)**

```powershell
docker compose up -d mysql
```

Requiere un `.env` en la raíz del repo (copiado de `.env.example`) con `MYSQL_ROOT_PASSWORD`, `MYSQL_DATABASE`, `MYSQL_USER`, `MYSQL_PASSWORD`, `MYSQL_PORT`. Ver [[ADR-010 - Docker Compose para MySQL Local]].

**2. Backend**

El backend (ver [[Compilar y Ejecutar el Backend con CMake]] para compilarlo) necesita las mismas variables de conexión como variables de entorno del proceso, y debe ejecutarse con el **directorio de trabajo en `backend/`** porque `main.cpp` carga el schema con una ruta relativa (`DatabaseManager::getInstance().migrateTables("../SQL/init.sql")`):

```powershell
$env:MYSQL_HOST="127.0.0.1"; $env:MYSQL_PORT="3306"
$env:MYSQL_USER="hermes_app"; $env:MYSQL_PASSWORD="..."; $env:MYSQL_DATABASE="hermes"
Set-Location backend
.\build\backend.exe
```

**3. Frontend**

```powershell
cd frontend
npm install
npm run dev
```

Sirve en `http://localhost:5173` (o el siguiente puerto libre si 5173 está ocupado — Vite incrementa solo). El proxy configurado en `vite.config.ts` reenvía `/api/*` a `http://127.0.0.1:18080/*` (el backend), así que no hace falta CORS en desarrollo.

# Why it matters

Los tres componentes son independientes y cada uno puede fallar por una razón distinta (contenedor no levantado, variables de entorno faltantes, directorio de trabajo incorrecto). Sin este orden y estas variables documentadas, el error real queda oculto detrás de síntomas confusos (ver Common Mistakes).

# Best Practices

- Verificar el contenedor con `docker ps --filter name=hermes-mysql --format "{{.Status}}"` antes de asumir que el backend puede conectar.
- Si el backend arranca por primera vez contra un volumen de MySQL vacío, `migrateTables` corre el schema completo; en arranques siguientes tolera que las columnas ya existan (ver [[MariaDB Connector-C (libmariadb)]], error 1060).
- Confirmar con `curl`/`Invoke-WebRequest` a `/health` que el backend realmente está escuchando antes de probar el frontend.

# Common Mistakes

- **Ejecutar `backend.exe` desde `backend/build/` en vez de `backend/`**: el path relativo al script de migración (`../SQL/init.sql`) apunta a un lugar distinto según el directorio de trabajo — desde `backend/build/` haría falta `../../SQL/init.sql`. El código actual asume `backend/` como cwd.
- **Cambiar las credenciales en `.env` después de que el volumen de MySQL ya se inicializó**: el usuario/password reales quedan siendo los originales hasta que se recrea el volumen (`docker compose down -v`) — ver [[Docker - Documentacion Oficial]].
- **Levantar procesos de larga duración (`backend.exe`, `npm run dev`) vía una herramienta de shell en background y esperar que respondan en `127.0.0.1`**: en este entorno de desarrollo se observó que lanzar estos procesos con backgrounding de una shell tipo Git Bash falla (exit code 127, o el proceso "arranca" pero no queda escuchando en ningún puerto real) — lanzarlos con `Start-Process` de PowerShell sí funciona de forma confiable. También se observó que Vite puede terminar bindeando solo en `[::1]` (IPv6) en vez de `127.0.0.1` (IPv4) si hay procesos previos ocupando los puertos bajos — probar con `http://localhost:<puerto>` (que resuelve ambas familias) en vez de forzar `127.0.0.1` si una conexión falla inesperadamente.
- **Probar `POST /upload` con `Invoke-WebRequest -Form` en Windows PowerShell 5.1**: el parámetro `-Form` no existe en esa versión (es de PowerShell 6+); hay que construir el body multipart a mano con un boundary y `Content-Disposition` explícitos.

# Hermes Usage

Es el flujo real usado durante el desarrollo del Sprint 2/3/8 para probar cada endpoint contra la base de datos real en vez de mocks, incluyendo la reproducción de bugs concretos (por ejemplo, el bypass de validación de audio con `Content-Type` falsificado, o la corrupción de columnas `DATETIME` al bindear resultados — ambos documentados en [[MariaDB Connector-C (libmariadb)]] y [[Sprint 3 - File Upload]]).

# Related Notes

- [[Compilar y Ejecutar el Backend con CMake]]
- [[ADR-010 - Docker Compose para MySQL Local]]
- [[MariaDB Connector-C (libmariadb)]]
- [[Sprint 8 - Frontend]]

# References

- docker-compose.yml (raíz del repositorio)
- backend/main.cpp
- frontend/vite.config.ts
- docs/API_REQUIREMENTS.md (Priority 1)
