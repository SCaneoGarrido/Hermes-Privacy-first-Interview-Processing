# Hermes

> Privacy-first Interview Processing

Hermes es una plataforma open source para procesar entrevistas de investigación cualitativa **de forma completamente local**: transcripción y, opcionalmente, corrección/estructuración por hablante, anonimización de información sensible y resumen — todo corriendo en la propia máquina del investigador, sin depender de ningún servicio cloud ni API externa.

Pensado para investigación que puede incluir información sensible (pacientes, profesionales de salud, identificadores personales), donde subir el audio a un servicio de terceros no es una opción aceptable.

**Estado actual: v0.1.0 — pre-release / early preview**, más los cambios sin publicar listados en [`CHANGELOG.md`](CHANGELOG.md) (sección *Unreleased*). Funciona de punta a punta con entrevistas reales, pero todavía no cumple el alcance completo planeado para una v1.0 (ver [Qué falta](#qué-falta-para-v10) y [Limitaciones conocidas](#limitaciones-conocidas) antes de usarlo con datos sensibles reales).

---

## Qué hace hoy

Con el backend y el frontend corriendo, más Ollama y un modelo de whisper.cpp descargados localmente, Hermes permite:

1. **Registrar una entrevista** (fecha y hora, tipo de entrevista, tipo de sujeto).
2. **Subir su audio o video** (mp3, wav, ogg, m4a o mp4; hasta 1 GB).
   - Validación por firma de bytes, no solo por extensión.
   - De un video solo se conserva el audio; el video original se borra al procesar (ADR-016).
   - Se puede **reemplazar el audio** de una entrevista: se borran el archivo anterior y la transcripción/resumen generados con él (con confirmación en la interfaz). No se permite mientras la entrevista se está procesando.
3. **Cargar palabras clave por entrevista** (opcional): Para mejora de precision de la transcripcion.
   - Se cargan desde un `.txt` separadas por coma, punto y coma o una por línea (máx. 100 términos de hasta 80 caracteres).
   - Guían a whisper y luego un paso con Ollama corrige sus variantes mal transcriptas (ej. "SILYES" → "SIGGES"), sin reescribir el resto del texto. Los cambios quedan listados en `glossary_changes.txt` (ADR-018).
   - Requiere Ollama ≥ 0.5 (salidas estructuradas).
4. **Procesarla en background** — cola de jobs con estados (`pending`/`running`/`completed`/`failed`), reintentable, con recuperación si el backend se reinicia a mitad de un job.
5. **Transcribirla localmente** con [whisper.cpp](https://github.com/ggml-org/whisper.cpp).
   - Audio normalizado antes vía FFmpeg (mono, 16 kHz, PCM16).
   - VAD Silero opcional para saltar silencios y ruido.
   - Detección de tramos donde whisper entra en bucle, que se re-transcriben; si no se recuperan, se marcan como `[audio no transcrito mm:ss-mm:ss]`.
6. **Opcionalmente, procesar la transcripción con un LLM local** vía [Ollama](https://ollama.com/) (familia Qwen). Ambas opciones vienen apagadas por defecto:
   - **Corregir y anonimizar** (`enhance_transcript`, experimental): corrección ortográfica/de puntuación, turnos `Investigador:`/`Entrevistado:` y reemplazo de nombres, lugares y organizaciones por marcadores (`[PERSONA_1]`, `[LUGAR_1]`…).
   - **Generar resumen** (`include_summary`): documento aparte de la transcripción.

   **Por defecto la transcripción es la salida plana de whisper y no está anonimizada** (ADR-017): el flujo asume que el investigador excluye la información sensible antes de grabar.
7. **Seguir el progreso** desde la interfaz: paso actual del procesamiento y tiempo transcurrido, con actualización automática.
8. **Leer la transcripción en la app** (ADR-019):
   - Portada con la ficha de la entrevista y un aviso de revisión.
   - Resumen, si se pidió.
   - Turnos por hablante (si se corrigió con IA) o párrafos con marca de tiempo `00:12:34` (salida plana de whisper).
   - Búsqueda dentro del texto y marcadores de anonimización resaltados para facilitar la revisión humana.
   - **"Imprimir / Guardar PDF"** desde el navegador: PDF A4 con portada, numeración de páginas y sin la interfaz alrededor.
9. **Descargar** la transcripción y el resumen como `.txt`.

Si Ollama no está corriendo o falla, la entrevista no se pierde: queda disponible la transcripción de whisper. Si se pidió anonimizar y esa fase falló, la transcripción entregada lo indica con un aviso al inicio del archivo (y en la portada de la vista de lectura).

### La interfaz

Frontend en React + TypeScript + Vite, sin librerías de UI y sin cargar nada de internet (fuentes del sistema, íconos SVG propios). Identidad visual inspirada en la Grecia clásica — Hermes, el mensajero.

- **Corpus de entrevistas**: listado con contadores, búsqueda, orden por fecha y filtros por estado; una acción principal por entrevista según su estado.
- **Detalle**: ficha, carga de audio (arrastrar y soltar), palabras clave, opciones de procesamiento, recorrido de los pasos del procesamiento, resultado y descargas.
- **Vista de lectura** de la transcripción, con impresión a PDF.
- Modo oscuro y claro según la preferencia del sistema.

## Qué falta para v1.0

El alcance original de v1.0 (definido al arrancar el proyecto) incluye piezas que todavía no existen:

- **Exportación formal** (DOCX para NVivo/Atlas.ti, JSON) — no empezada. Bloqueada a propósito hasta que la anonimización sea confiable. La vista de lectura con "Guardar PDF" es para leer y revisar, no reemplaza a Export.
- **Endpoint de configuración** — no empezado; hoy toda la configuración es por variables de entorno.
- **Tests automatizados** — no hay tests Catch2 todavía, solo verificación manual contra entrevistas reales.
- **Instalador de Windows** — no empezado.
- **Aceleración por GPU** (Vulkan para whisper.cpp) — investigado y planeado, no implementado; hoy todo corre en CPU.
- **Progreso de subida** del archivo — la subida muestra "Subiendo…" sin porcentaje.

El detalle sprint por sprint vive en [`.ai/ROADMAP.md`](.ai/ROADMAP.md).

## Limitaciones conocidas

Verificado contra entrevistas reales (no solo audio de prueba sintético). Lo que se encontró:

- **La anonimización automática (opcional) todavía no es confiable.** En la validación de una entrevista real reemplazó sustantivos comunes ("paciente", "médico") y anidó marcadores, y en la corrección se perdió ~3% del texto en los cortes de bloque; por eso está apagada por defecto (ADR-017). Además, no tiene 100% de recall: en pruebas reales quedaron sin anonimizar nombres de figuras públicas mencionadas de pasada. **No asumir que una transcripción "anonimizada" está realmente libre de PII sin revisión humana** — es la razón principal por la que Export sigue bloqueado.
- **La atribución de hablante (`Investigador:`/`Entrevistado:`) es una aproximación heurística del LLM sobre texto, no diarización acústica real.** Tiene errores esperables en diálogo rápido o con turnos muy cortos.
- whisper.cpp puede alucinar texto en otro idioma en tramos de audio poco claros.
- El paso de palabras clave puede dejar algún bloque sin revisar si la respuesta del modelo se corta (queda registrado en el log).
- **El backend escucha en todas las interfaces de red (`0.0.0.0:18080`) y no tiene autenticación** (está fuera de alcance). Cualquier equipo de la misma red local puede consultar la API y descargar transcripciones. Usarlo en una red de confianza, o cambiar el `bindaddr` a `127.0.0.1` en `backend/main.cpp` si el frontend corre en la misma máquina.
- Todo corre en CPU hoy — una entrevista de ~70 minutos tarda del orden de 20-25 minutos en procesarse completa.

El detalle y el backlog priorizado de estos hallazgos están en `hermes-vault-knowledge/08 AI/Ollama Integration Strategy.md`.

---

## Arquitectura

Monolito modular en C++20, con el frontend como cliente puro de la API REST — toda la lógica de negocio vive en el backend.

```
frontend/    React + TypeScript + Vite
backend/     API (Crow) → Services → Repositories → MySQL / whisper.cpp / Ollama
SQL/         Esquema de base de datos
docs/        Requisitos de API, validaciones manuales y mockups de diseño
.ai/         Contexto del proyecto para trabajo asistido por IA (ADRs, roadmap, prompts)
hermes-vault-knowledge/  Base de conocimiento (Obsidian) — arquitectura, ADRs, hallazgos, changelog por sesión
```

Cada dependencia externa está detrás de una interfaz (`ITranscriber` → `WhisperTranscriber`, `ILLMClient` → `OllamaClient`, `IInterviewRepository` → `MySqlInterviewRepository`, etc.) — la lógica de negocio no depende directamente de Crow, MySQL, whisper.cpp ni Ollama.

Decisiones de arquitectura documentadas como ADRs en [`.ai/DECISIONS.md`](.ai/DECISIONS.md).

### Dónde quedan los datos

Todo queda en la máquina local, dentro de `backend/` (ambas carpetas están en `.gitignore`):

| Carpeta | Contenido |
|---|---|
| `uploads/` | Audio fuente de cada entrevista (un archivo por entrevista) |
| `storage/interviews/<id>/` | Audio normalizado, `transcript_raw.json` (segmentos con tiempos), `transcript_final.txt`, resumen, `glossary_changes.txt` |

Al **eliminar** una entrevista se borran su audio y su carpeta en `storage/`. Al **reemplazar** el audio se borran el anterior y los resultados generados con él. Un archivo subido que se rechaza no queda en disco.

## Stack

| | |
|---|---|
| Backend | C++20, [Crow](https://crowcpp.org/) |
| Base de datos | MySQL 8 (Docker Compose), cliente libmariadb |
| Reconocimiento de voz | whisper.cpp (+ VAD Silero opcional) |
| Audio | FFmpeg (estático vía vcpkg) |
| LLM | Ollama (Qwen), cliente cpp-httplib |
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
- **[Ollama](https://ollama.com/download)** ≥ 0.5 instalado y corriendo localmente, con un modelo Qwen descargado: `ollama pull qwen2.5:7b` (o `qwen2.5:3b-instruct` en hardware más limitado). Solo se usa si se activan las opciones de IA o hay palabras clave.
- Un modelo de **whisper.cpp** en formato `ggml` (ej. `ggml-small.bin`) descargado desde [huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) o vía `models/download-ggml-model.sh` del repo de whisper.cpp.
- **Node.js** (para el frontend).

### 1. Base de datos

```powershell
cp .env.example .env   # ajustar credenciales
docker compose up -d
```

El esquema se crea y migra solo al arrancar el backend (`SQL/init.sql`). Si MySQL se reinicia con el backend corriendo, el backend se reconecta solo.

### 2. Backend

```powershell
cd backend
cmake -S . -B build
cmake --build build
.\build\backend.exe
```

`cmake -S . -B build` instala automáticamente las dependencias declaradas en `vcpkg.json` (manifest mode) — no hace falta correr `vcpkg install` a mano. El backend escucha en el puerto `18080` (`/api/v1/...`) y escribe su log en `backend/API.log`.

Colocá el modelo de whisper.cpp en `backend/models/ggml-small.bin`, o configurá `WHISPER_MODEL_PATH` con la ruta que uses.

Recomendado: el modelo VAD Silero (`ggml-silero-v5.1.2.bin`, ~1 MB, de [huggingface.co/ggml-org/whisper-vad](https://huggingface.co/ggml-org/whisper-vad)) en `backend/models/`. Hace que whisper salte silencios y ruido, donde suele alucinar frases repetidas. Sin él el backend funciona igual, pero lo avisa en el log.

Para recompilar, cerrá antes el `backend.exe` en ejecución: si está abierto, el enlazado falla con `Permission denied`. Corré una sola instancia del backend a la vez.

### 3. Frontend

```powershell
cd frontend
npm install
npm run dev
```

El dev server de Vite reenvía `/api/*` al backend (sin CORS). **El destino del proxy está fijo en `frontend/vite.config.ts`** (`target`) y hoy apunta a una IP de red local; si el backend corre en la misma máquina, cambialo a `http://127.0.0.1:18080`.

### Variables de entorno relevantes (backend)

| Variable | Default | Uso |
|---|---|---|
| `MYSQL_HOST` / `MYSQL_USER` / `MYSQL_PASSWORD` / `MYSQL_DATABASE` / `MYSQL_PORT` | ver `docker-compose.yml` | Conexión a MySQL |
| `WHISPER_MODEL_PATH` | `./models/ggml-small.bin` | Modelo de whisper.cpp |
| `WHISPER_LANGUAGE` | `es` | Idioma forzado para la transcripción |
| `WHISPER_VAD_MODEL_PATH` | `./models/ggml-silero-v5.1.2.bin` | Modelo VAD (opcional; sin el archivo no hay VAD) |
| `OLLAMA_BASE_URL` | `http://localhost:11434` | Endpoint de Ollama |
| `OLLAMA_MODEL` | `qwen2.5:7b` | Modelo Qwen a usar |
| `WORKER_POOL_SIZE` | `1` | Threads del worker pool de procesamiento |

### Endpoints principales

```
GET    /api/v1/health
POST   /api/v1/upload                            (multipart "file" + header interview_id)
POST   /api/v1/interview
GET    /api/v1/interviews
GET    /api/v1/interview/:id
DELETE /api/v1/interview/:id
POST   /api/v1/interview/:id/process             ({ include_summary, enhance_transcript })
PUT    /api/v1/interview/:id/keywords
GET    /api/v1/interview/:id/transcript          (transcripción estructurada para la vista de lectura)
GET    /api/v1/interview/:id/download/transcript
GET    /api/v1/interview/:id/download/summary
```

Todas las respuestas JSON usan el sobre `{ success, data, error }`. Contrato completo, códigos de error incluidos, en [`docs/API_REQUIREMENTS.md`](docs/API_REQUIREMENTS.md).

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
