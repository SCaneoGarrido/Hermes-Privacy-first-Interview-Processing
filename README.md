# Hermes

> Privacy-first Interview Processing

Hermes es una plataforma open source para procesar entrevistas de investigación cualitativa **de forma completamente local**: transcripción, corrección/estructuración por hablante, anonimización de información sensible y (opcionalmente) resumen — todo corriendo en la propia máquina del investigador, sin depender de ningún servicio cloud ni API externa.

Pensado para investigación que puede incluir información sensible (pacientes, profesionales de salud, identificadores personales), donde subir el audio a un servicio de terceros no es una opción aceptable.

**Estado actual: v0.1.0 — pre-release / early preview.** Funciona de punta a punta con entrevistas reales, pero todavía no cumple el alcance completo planeado para una v1.0 (ver [Qué falta](#qué-falta-para-v10) y [Limitaciones conocidas](#limitaciones-conocidas) más abajo antes de usarlo con datos sensibles reales).

---

## Qué hace hoy

Con el backend y el frontend corriendo, más Ollama y un modelo de whisper.cpp descargados localmente, Hermes permite:

1. **Subir un audio de entrevista** (validación por firma de bytes, no solo extensión).
2. **Procesarlo en background** — cola de jobs con estados (`pending`/`running`/`completed`/`failed`), reintentable, con recuperación si el backend se reinicia a mitad de un job.
3. **Transcribirlo localmente** con [whisper.cpp](https://github.com/ggml-org/whisper.cpp) (audio normalizado antes vía FFmpeg: mono, 16kHz, PCM16).
4. **Mejorar la transcripción con un LLM local** vía [Ollama](https://ollama.com/) (familia Qwen), en tres fases:
   - Corrección ortográfica/de puntuación + estructuración por hablante (`Investigador:`/`Entrevistado:`).
   - Anonimización de nombres, lugares y organizaciones (tabla de sustitución consistente en toda la entrevista).
   - Resumen final (opcional, `include_summary`, apagado por defecto).
5. **Seguir el progreso** desde el frontend (paso actual, tiempo transcurrido).
6. **Descargar** la transcripción final y, si se pidió, el resumen, como `.txt`.

Si Ollama no está corriendo o falla, la entrevista no se pierde: queda disponible la transcripción cruda de whisper sin diarizar (degradación con gracia).

## Qué falta para v1.0

El alcance original de v1.0 (definido al arrancar el proyecto) incluye piezas que todavía no existen:

- **Exportación** (DOCX/PDF/JSON) — no empezada. Bloqueada a propósito hasta que la anonimización sea confiable (ver abajo).
- **Endpoint de configuración** — no empezado; hoy toda la configuración es por variables de entorno.
- **Tests automatizados** — no hay tests Catch2 todavía, solo verificación manual contra entrevistas reales.
- **Instalador de Windows** — no empezado.
- **Aceleración por GPU** (Vulkan para whisper.cpp) — investigado y planeado, no implementado; hoy todo corre en CPU.

El detalle sprint por sprint vive en [`.ai/ROADMAP.md`](.ai/ROADMAP.md).

## Limitaciones conocidas

Verificado contra entrevistas reales (no solo audio de prueba sintético). Lo que se encontró:

- **La anonimización automática no tiene 100% de recall.** En pruebas reales quedaron sin anonimizar nombres de figuras públicas mencionadas de pasada. **No asumir que una transcripción "anonimizada" está realmente libre de PII sin revisión humana** — es la razón principal por la que Export sigue bloqueado.
- **La atribución de hablante (`Investigador:`/`Entrevistado:`) es una aproximación heurística del LLM sobre texto, no diarización acústica real.** Tiene errores esperables en diálogo rápido o con turnos muy cortos.
- whisper.cpp puede alucinar texto en otro idioma en tramos de audio poco claros.
- Todo corre en CPU hoy — una entrevista de ~70 minutos tarda del orden de 20-25 minutos en procesarse completa.

El detalle y el backlog priorizado de estos hallazgos están en `hermes-vault-knowledge/08 AI/Ollama Integration Strategy.md`.

---

## Arquitectura

Monolito modular en C++20, con el frontend como cliente puro de la API REST — toda la lógica de negocio vive en el backend.

```
frontend/    React + TypeScript + Vite
backend/     API (Crow) → Services → Repositories → MySQL / whisper.cpp / Ollama
SQL/         Esquema de base de datos
docs/        Requisitos de API y decisiones técnicas
.ai/         Contexto del proyecto para trabajo asistido por IA
hermes-vault-knowledge/  Base de conocimiento (Obsidian) — arquitectura, ADRs, hallazgos
```

Cada dependencia externa está detrás de una interfaz (`ITranscriber` → `WhisperTranscriber`, `ILLMClient` → `OllamaClient`, `IInterviewRepository` → `MySqlInterviewRepository`, etc.) — la lógica de negocio no depende directamente de Crow, MySQL, whisper.cpp ni Ollama.

Decisiones de arquitectura documentadas como ADRs en [`.ai/DECISIONS.md`](.ai/DECISIONS.md).

## Stack

| | |
|---|---|
| Backend | C++20, [Crow](https://crowcpp.org/) |
| Base de datos | MySQL 8 (Docker Compose) |
| Reconocimiento de voz | whisper.cpp |
| LLM | Ollama (Qwen) |
| Frontend | React + TypeScript + Vite |
| Build | CMake + vcpkg (manifest mode) |
| Serialización | nlohmann/json |
| Logging | spdlog |

---

## Cómo correrlo

### Requisitos previos

- **MinGW-w64** en el `PATH` (g++, gcc, ninja, cmake) — el proyecto compila con MinGW, no necesita Visual Studio.
- Variable de entorno `VCPKG_ROOT` apuntando a una instalación local de [vcpkg](https://vcpkg.io/).
- **Docker** (para MySQL vía `docker-compose.yml`).
- **[Ollama](https://ollama.com/download)** instalado y corriendo localmente, con un modelo Qwen descargado: `ollama pull qwen2.5:7b` (o `qwen2.5:3b-instruct` en hardware más limitado).
- Un modelo de **whisper.cpp** en formato `ggml` (ej. `ggml-small.bin`) descargado desde [huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) o vía `models/download-ggml-model.sh` del repo de whisper.cpp.
- **Node.js** (para el frontend).

### 1. Base de datos

```powershell
cp .env.example .env   # ajustar credenciales
docker compose up -d
```

### 2. Backend

```powershell
cd backend
cmake -S . -B build
cmake --build build
.\build\backend.exe
```

`cmake -S . -B build` instala automáticamente las dependencias declaradas en `vcpkg.json` (manifest mode) — no hace falta correr `vcpkg install` a mano. El backend escucha en `http://127.0.0.1:18080` (`/api/v1/...`).

Coloca el modelo de whisper.cpp en `backend/models/ggml-small.bin`, o configurá `WHISPER_MODEL_PATH` con la ruta que uses.

### 3. Frontend

```powershell
cd frontend
npm install
npm run dev
```

### Variables de entorno relevantes (backend)

| Variable | Default | Uso |
|---|---|---|
| `MYSQL_HOST` / `MYSQL_USER` / `MYSQL_PASSWORD` / `MYSQL_DATABASE` / `MYSQL_PORT` | ver `docker-compose.yml` | Conexión a MySQL |
| `WHISPER_MODEL_PATH` | `./models/ggml-small.bin` | Modelo de whisper.cpp |
| `WHISPER_LANGUAGE` | `es` | Idioma forzado para la transcripción |
| `OLLAMA_BASE_URL` | `http://localhost:11434` | Endpoint de Ollama |
| `OLLAMA_MODEL` | `qwen2.5:7b` | Modelo Qwen a usar |
| `WORKER_POOL_SIZE` | `1` | Threads del worker pool de procesamiento |

### Endpoints principales

```
GET    /api/v1/health
POST   /api/v1/upload
POST   /api/v1/interview
GET    /api/v1/interviews
GET    /api/v1/interview/:id
DELETE /api/v1/interview/:id
POST   /api/v1/interview/:id/process
GET    /api/v1/interview/:id/download/transcript
GET    /api/v1/interview/:id/download/summary
```

Contrato completo en [`docs/API_REQUIREMENTS.md`](docs/API_REQUIREMENTS.md).

---

## Filosofía del proyecto

- **Privacy First** — información sensible nunca sale de la máquina local salvo configuración explícita del usuario.
- **Local First** — funciona sin conexión a Internet.
- **API First** — toda la lógica de negocio se expone vía REST; el frontend es solo un cliente.
- **Monolito modular** — no microservicios; un único ejecutable con módulos independientes.
- **Clean Architecture** — los frameworks son detalles de implementación, la lógica de negocio no depende de Crow, MySQL, whisper.cpp ni Ollama.
- **Mantenibilidad sobre ingenio** — código legible antes que optimizaciones complejas.

### Fuera de alcance

No se va a agregar (salvo decisión explícita en contrario): autenticación/usuarios/roles, multi-tenant, procesamiento distribuido, Kubernetes, microservicios, message brokers, ni integración con servicios cloud (Azure/AWS/GCP/OpenAI API).

## Licencia

[MIT](LICENSE)
