# Hermes

> Privacy-first Interview Processing

Hermes es una plataforma open source para procesar entrevistas de investigación cualitativa **de forma completamente local**: transcripción y, opcionalmente, corrección/estructuración por hablante, anonimización de información sensible y resumen — todo corriendo en la propia máquina del investigador, sin depender de ningún servicio cloud ni API externa.

Pensado para investigación que puede incluir información sensible (pacientes, profesionales de salud, identificadores personales), donde subir el audio a un servicio de terceros no es una opción aceptable.

**Estado actual: v0.1.0 — pre-release / early preview**, más los cambios sin publicar listados en [`CHANGELOG.md`](CHANGELOG.md) (sección *Unreleased*). Funciona de punta a punta con entrevistas reales, pero todavía no cumple el alcance completo planeado para una v1.0 (ver [Qué falta](#qué-falta-para-v10) y [Limitaciones conocidas](#limitaciones-conocidas) antes de usarlo con datos sensibles reales).

---

## Qué hace hoy

Con el backend y el frontend corriendo, más Ollama y un modelo de whisper.cpp descargados localmente, Hermes permite:

1. **Registrar una entrevista** (fecha y hora, tipo de entrevista, tipo de sujeto). El tipo de sujeto (ej. "Monitor GES") es el nombre con el que figura el entrevistado en la transcripción.
2. **Subir su audio o video** (mp3, wav, ogg, m4a o mp4; hasta 1 GB).
   - Validación por firma de bytes, no solo por extensión.
   - De un video solo se conserva el audio; el video original se borra al procesar (ADR-016).
   - Se puede **reemplazar el audio** de una entrevista: se borran el archivo anterior y la transcripción/resumen generados con él (con confirmación en la interfaz). No se permite mientras la entrevista se está procesando.
3. **Cargar palabras clave por entrevista** (opcional): Para mejora de precision de la transcripcion.
   - Se cargan desde un `.txt` separadas por coma, punto y coma o una por línea (máx. 100 términos de hasta 80 caracteres).
   - Guían a whisper y luego un paso con Ollama corrige sus variantes mal transcriptas (ej. "SILYES" → "SIGGES"), sin reescribir el resto del texto. Los cambios quedan listados en `glossary_changes.txt` (ADR-018).
   - Requiere Ollama ≥ 0.5 (salidas estructuradas).
4. **Procesarla en background** — cola de jobs con estados (`pending`/`running`/`completed`/`failed`), reintentable, con recuperación si el backend se reinicia a mitad de un job.
5. **Transcribirla localmente** con [whisper.cpp](https://github.com/ggml-org/whisper.cpp) (ADR-020).
   - En GPU (Vulkan: AMD, NVIDIA o Intel) usa el modelo `large-v3` con beam search; sin GPU cae a CPU solo.
   - Audio normalizado antes vía FFmpeg (mono, 16 kHz, PCM16).
   - VAD Silero opcional para saltar silencios y ruido.
   - Detección de tramos donde whisper entra en bucle, que se re-transcriben; si no se recuperan, se marcan como `[audio no transcrito mm:ss-mm:ss]`.
6. **Identificar quién habla** por la voz (diarización acústica local con sherpa-onnx, ADR-021).
   - Cada turno se etiqueta como **Investigador** o con el **tipo de sujeto** de la entrevista.
   - Quién es el investigador se decide automáticamente (quien hace las preguntas) y se corrige desde el editor si salió al revés.
   - Opcional: sin la DLL y los modelos de sherpa-onnx, se transcribe igual, sin hablantes.
7. **Opcionalmente, procesar la transcripción con un LLM local** vía [Ollama](https://ollama.com/) (familia Qwen). Ambas opciones vienen apagadas por defecto:
   - **Corregir y anonimizar** (`enhance_transcript`, experimental): corrección ortográfica y de puntuación turno por turno (sin tocar los hablantes) y reemplazo de nombres, lugares y organizaciones por marcadores (`[PERSONA_1]`, `[LUGAR_1]`…).
   - **Generar resumen** (`include_summary`): documento aparte de la transcripción.

   **Por defecto la transcripción es la salida plana de whisper y no está anonimizada** (ADR-017): el flujo asume que el investigador excluye la información sensible antes de grabar.
8. **Seguir el progreso** desde la interfaz: paso actual del procesamiento y tiempo transcurrido, con actualización automática.
9. **Leer la transcripción en la app** (ADR-019):
   - Portada con la ficha de la entrevista y un aviso de revisión.
   - Resumen, si se pidió.
   - Turnos por hablante con su marca de tiempo, o párrafos con marca de tiempo si no hay hablantes.
   - Búsqueda dentro del texto y marcadores de anonimización resaltados para facilitar la revisión humana.
   - **"Imprimir / Guardar PDF"** desde el navegador: PDF A4 con portada, numeración de páginas y sin la interfaz alrededor.
10. **Editar la transcripción** desde la vista de lectura (ADR-022).
    - Texto y hablante de cada turno; dividir, unir y eliminar turnos.
    - **Intercambiar los dos hablantes** de toda la entrevista con un clic.
    - Las ediciones se guardan aparte: la versión original se conserva y se puede restaurar. Volver a procesar la entrevista descarta las ediciones (la interfaz lo advierte antes).
11. **Escuchar la entrevista mientras se lee o se edita** (ADR-023).
    - Reproductor debajo del texto que resalta el turno que se está escuchando y puede seguirlo automáticamente.
    - Clic en una marca de tiempo para escuchar desde ahí.
    - Saltos de 5 s y velocidad 0,75×-1,5×. Atajos: Alt+K pausa, Alt+J / Alt+L ±5 s; funcionan mientras se escribe en el editor.
    - El audio se sirve por rangos: saltar a cualquier minuto no descarga el archivo entero.
12. **Descargar** la transcripción (versión vigente, con nombres de hablante) y el resumen como `.txt`.

Si Ollama no está corriendo o falla, la entrevista no se pierde: queda disponible la transcripción de whisper. Si se pidió anonimizar y esa fase falló, la transcripción entregada lo indica con un aviso al inicio del archivo (y en la portada de la vista de lectura).

### La interfaz

Frontend en React + TypeScript + Vite, sin librerías de UI y sin cargar nada de internet (fuentes del sistema, íconos SVG propios). Identidad visual inspirada en la Grecia clásica — Hermes, el mensajero.

- **Corpus de entrevistas**: listado con contadores, búsqueda, orden por fecha y filtros por estado; una acción principal por entrevista según su estado.
- **Detalle**: ficha, carga de audio (arrastrar y soltar), palabras clave, opciones de procesamiento, recorrido de los pasos del procesamiento, resultado y descargas.
- **Vista de lectura** de la transcripción, con impresión a PDF y modo edición.
- Modo oscuro y claro según la preferencia del sistema.

#### Capturas

En las capturas, los datos de las entrevistas reales están tapados.

**Corpus de entrevistas:** listado con contadores, búsqueda y filtros por estado.

![Corpus de entrevistas](<docs/Capturas/Corpus de entrevistas.png>)

**Registrar una entrevista:** el *tipo de sujeto* es el nombre con el que figura el entrevistado en la transcripción.

![Crear entrevista](<docs/Capturas/Crear entrevista.png>)

**Configuración de la entrevista:** carga de audio, palabras clave y opciones de procesamiento.

![Configuración de entrevista](<docs/Capturas/Configuracion de entrevista (carga de audio, opciones de procesamiento, palabras clave).png>)

**Detalle durante el procesamiento:** recorrido de los pasos, incluida la identificación de hablantes.

![Detalle de entrevista](<docs/Capturas/Detalle de Entrevistas.png>)

## Qué falta para v1.0

El alcance original de v1.0 (definido al arrancar el proyecto) incluye piezas que todavía no existen:

- **Exportación formal** (DOCX para NVivo/Atlas.ti, JSON) — no empezada. Bloqueada a propósito hasta que la anonimización sea confiable. La vista de lectura con "Guardar PDF" es para leer y revisar, no reemplaza a Export.
- **Endpoint de configuración** — no empezado; hoy toda la configuración es por variables de entorno.
- **Tests automatizados** — no hay tests Catch2 todavía, solo verificación manual contra entrevistas reales.
- **Instalador de Windows** — no empezado.
- **Progreso de subida** del archivo — la subida muestra "Subiendo…" sin porcentaje.

El detalle sprint por sprint vive en [`.ai/ROADMAP.md`](.ai/ROADMAP.md).

## Limitaciones conocidas

Verificado contra entrevistas reales (no solo audio de prueba sintético). Lo que se encontró:

- **La anonimización automática (opcional) todavía no es confiable.** En la validación de una entrevista real reemplazó sustantivos comunes ("paciente", "médico") y anidó marcadores, y en la corrección se perdió ~3% del texto en los cortes de bloque; por eso está apagada por defecto (ADR-017). Además, no tiene 100% de recall: en pruebas reales quedaron sin anonimizar nombres de figuras públicas mencionadas de pasada. **No asumir que una transcripción "anonimizada" está realmente libre de PII sin revisión humana** — es la razón principal por la que Export sigue bloqueado.
- **La diarización acústica no es perfecta.**
  - Atribuye mal intervenciones muy cortas ("ya", "mhm"), habla superpuesta y audio de un solo micrófono con mucho eco.
  - Está pensada para dos personas: con más de dos, los demás se asignan al rol más parecido.
  - Quién es el investigador se deduce de quién hace más preguntas: en entrevistas atípicas puede salir invertido. Se corrige con «Intercambiar» en el editor.
- whisper.cpp puede alucinar texto en otro idioma en tramos de audio poco claros.
- El paso de palabras clave puede dejar algún bloque sin revisar si la respuesta del modelo se corta (queda registrado en el log).
- **El backend escucha en todas las interfaces de red (`0.0.0.0:18080`) y no tiene autenticación** (está fuera de alcance). Cualquier equipo de la misma red local puede consultar la API, descargar transcripciones y escuchar el audio de las entrevistas. Usarlo en una red de confianza, o cambiar el `bindaddr` a `127.0.0.1` en `backend/main.cpp` si el frontend corre en la misma máquina.
- Sin GPU, `large-v3` en CPU es varias veces más lento que `small`: en una máquina solo CPU conviene `WHISPER_MODEL_PATH=./models/ggml-small.bin`. La diarización corre en CPU (unos 4 minutos por cada 35 minutos de audio).

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
| `storage/interviews/<id>/` | Audio normalizado, `transcript_raw.json` (salida cruda de whisper, con tiempos por palabra), `transcript_segments.json` (transcripción entregada, con hablantes), `transcript_edited.json` (ediciones manuales, si las hay), `transcript_final.txt`, resumen, `glossary_changes.txt` |

Al **eliminar** una entrevista se borran su audio y su carpeta en `storage/`. Al **reemplazar** el audio se borran el anterior y los resultados generados con él. Un archivo subido que se rechaza no queda en disco.

## Stack

| | |
|---|---|
| Backend | C++20, [Crow](https://crowcpp.org/) |
| Base de datos | MySQL 8 (Docker Compose), cliente libmariadb |
| Reconocimiento de voz | whisper.cpp con backend Vulkan (+ VAD Silero opcional) |
| Diarización | sherpa-onnx (API C, DLL cargada en runtime) |
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
- Un modelo de **whisper.cpp** en formato `ggml` (`ggml-large-v3.bin` con GPU, `ggml-small.bin` solo CPU) descargado desde [huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) o vía `models/download-ggml-model.sh` del repo de whisper.cpp.
- Para GPU: un driver de video actualizado. Provee `vulkan-1.dll`, que el backend necesita para arrancar desde ADR-020.
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

**Primera compilación con Vulkan:** vcpkg compila shaderc (~15 min). El MinGW del proyecto tiene que estar **primero** en el `PATH`. Si antes aparece otro MinGW (el de Git en `/mingw64/bin`, msys64…), el compilador de shaders falla en silencio y el enlazado termina con miles de `undefined reference to '..._data'` (ver ADR-020).

Colocá el modelo de whisper.cpp en `backend/models/ggml-large-v3.bin`, o configurá `WHISPER_MODEL_PATH` con la ruta que uses.

**Diarización (recomendado).** Identifica investigador y entrevistado por la voz. Requiere archivos que se descargan una sola vez:
- de la release [sherpa-onnx v1.13.8](https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.13.8), el archivo `sherpa-onnx-v1.13.8-win-x64-shared-MD-Release-no-tts.tar.bz2`. Copiá `lib/sherpa-onnx-c-api.dll`, `lib/onnxruntime.dll` y `lib/onnxruntime_providers_shared.dll` a `backend/sherpa-onnx/`. Tiene que ser esa versión exacta: el header vendorizado en `backend/third_party/sherpa-onnx/` corresponde a ella;
- [`sherpa-onnx-pyannote-segmentation-3-0.tar.bz2`](https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-segmentation-models/sherpa-onnx-pyannote-segmentation-3-0.tar.bz2). Su `model.onnx` va a `backend/models/sherpa-onnx-pyannote-segmentation-3-0.onnx`;
- [`3dspeaker_speech_campplus_sv_zh_en_16k-common_advanced.onnx`](https://github.com/k2-fsa/sherpa-onnx/releases/download/speaker-recongition-models/3dspeaker_speech_campplus_sv_zh_en_16k-common_advanced.onnx), a `backend/models/`.

Sin estos archivos el backend funciona igual: entrega la transcripción sin hablantes y lo avisa en el log.

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
| `WHISPER_MODEL_PATH` | `./models/ggml-large-v3.bin` | Modelo de whisper.cpp |
| `WHISPER_LANGUAGE` | `es` | Idioma forzado para la transcripción |
| `WHISPER_VAD_MODEL_PATH` | `./models/ggml-silero-v5.1.2.bin` | Modelo VAD (opcional; sin el archivo no hay VAD) |
| `WHISPER_USE_GPU` | `auto` | `auto` usa la GPU (Vulkan) si hay; `1` la fuerza; `0` siempre CPU |
| `DIARIZATION_LIB_DIR` | `./sherpa-onnx` | Carpeta con las DLL de sherpa-onnx |
| `DIARIZATION_SEGMENTATION_MODEL` | `./models/sherpa-onnx-pyannote-segmentation-3-0.onnx` | Modelo de segmentación de hablantes |
| `DIARIZATION_EMBEDDING_MODEL` | `./models/3dspeaker_speech_campplus_sv_zh_en_16k-common_advanced.onnx` | Modelo de embeddings de voz |
| `DIARIZATION_NUM_SPEAKERS` | `0` | `0`: agrupar por umbral (recomendado); `N > 0`: forzar N grupos |
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
PUT    /api/v1/interview/:id/transcript          (guarda la versión editada: { blocks })
DELETE /api/v1/interview/:id/transcript/edits    (descarta las ediciones, vuelve al original)
GET    /api/v1/interview/:id/audio               (audio normalizado para el reproductor; soporta Range → 206)
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
