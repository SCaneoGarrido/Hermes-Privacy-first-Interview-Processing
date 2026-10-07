# Requisitos de API para el Frontend

Este documento describe, desde el punto de vista del frontend, qué expone
hoy la API de Crow para soportar el flujo mínimo: **crear una entrevista →
asociarle un audio → enviarla a procesar**.

El frontend está construido contra el contrato descripto acá (ver
`frontend/src/api/`). Si algún endpoint dejara de existir o cambiara de
forma, el `CROW_CATCHALL_ROUTE` sigue devolviendo
`{"success":false,"error":{"code":"NOT_FOUND",...}}` en vez de romper la UI
en silencio.

**Todas las rutas del backend están versionadas bajo el prefijo `/api/v1`**
(`backend/main.cpp`, adoptado 2026-07-28 — ver ADR-013 en `.ai/DECISIONS.md`).
Los encabezados de esta sección muestran el path real, ya con el prefijo.

Todas las respuestas siguen el contrato ya definido en
`backend/api/rules/contract.md`:

```json
{
  "success": true | false,
  "data": <objeto | array | null>,
  "error": { "code": "SCREAMING_SNAKE_CASE", "message": "..." } | null
}
```

---

## 1. Endpoints implementados

### `GET /api/v1/health`
Se usa para el indicador de estado del backend en el header del frontend.

### `POST /api/v1/interview`
Crea una entrevista. Body:
```json
{ "date": "2026-07-28 10:00:00", "type": "tecnica", "subject_type": "candidato" }
```
Responde `data: { "code": "CREATED", "id": 7 }`. `InterviewController::handleInterviewRegistration`
delega en `InterviewService::createInterview`, que usa
`IInterviewRepository::create` (`MySqlInterviewRepository` internamente
llama a `DatabaseManager::executePrepared(..., /*return_id=*/true)` →
`mysql_stmt_insert_id`). Ver ADR-012 en `.ai/DECISIONS.md`.

### `POST /api/v1/upload`
Sube un audio (o video .mp4) y lo asocia a una entrevista. Requiere:
- Multipart con campo `file` (mp3/wav/ogg/m4a, o `video/mp4`), validado contra
  la firma real de bytes del archivo (no solo el `Content-Type` declarado — ver
  `AudioSignature.h`).
- Header **`interview_id`** (entero, validado por `HeaderIdGuard`).
- Tamaño máximo 1 GiB (`Config::MAX_UPLOAD_BYTES`): si se supera responde
  `413 PAYLOAD_TOO_LARGE`.

Si es un video, el job de procesamiento extrae su audio, lo deja como nueva
fuente de la entrevista (`uploads/<uuid>.wav`, formato `.wav`) y **borra el
video original** (ADR-016).

Responde `data: { "filename", "path", "code": "ACCEPTED" }`. Al guardar el
audio, además actualiza `interviews.status` a `pending_processing`.

Errores (vía `InterviewService::canAttachAudio`, chequeado **antes** de
escribir el archivo en disco):
- `404 NOT_FOUND` si la entrevista no existe.
- `409 INTERVIEW_BUSY` si la entrevista está en `processing` o tiene un job
  encolado: el audio no puede cambiar en medio de un procesamiento.
- `500 INTERNAL_SERVER_ERROR` si falla el registro en la BD. En cualquier
  rechazo posterior a guardar, el archivo recién subido se borra: un upload
  rechazado nunca queda huérfano en `./uploads`.

**Reemplazo:** si la entrevista ya tenía audio, se actualiza la fila de
`interviews_audio` (no se inserta otra), se borra el archivo anterior y se
descarta el resultado previo (fila de `interview_results` y la carpeta
`storage/interviews/<id>`), porque ya no corresponde al audio nuevo. El
frontend pide confirmación antes de reemplazar.

### `GET /api/v1/interviews` — listado
`InterviewController::getInterviews`. Devuelve un array leyendo la columna
`status` directamente (ver sección 2):

```json
{
  "success": true,
  "data": [
    {
      "id": 7,
      "date": "2026-07-28 10:00:00",
      "type": "tecnica",
      "subject_type": "candidato",
      "status": "pending_audio",
      "created_at": "2026-07-28 10:00:05"
    }
  ],
  "error": null
}
```

### `GET /api/v1/interview/:id` — detalle
`InterviewController::getInterview`. Devuelve la entrevista con su audio y
resultado si existen (`null` si no, coherente con el `UNIQUE(interview_id)`
de `SQL/init.sql`):

```json
{
  "success": true,
  "data": {
    "id": 7,
    "date": "2026-07-28 10:00:00",
    "type": "tecnica",
    "subject_type": "candidato",
    "status": "pending_processing",
    "audio": { "path": "./uploads/....wav", "format": ".wav", "size": 32 },
    "result": null
  },
  "error": null
}
```
`404 NOT_FOUND` si el id no existe.

### `PUT /api/v1/interview/:id/keywords` — glosario de palabras clave
Reemplaza el glosario de la entrevista (ADR-018). Body: `{"keywords": ["SIGGES", "GES", "FONASA"]}`. Una
lista vacía lo borra.
- El backend recorta espacios, ignora vacíos y deduplica sin distinguir mayúsculas.
- `400 INVALID_KEYWORDS`: body sin la lista, elementos que no son texto, más de 100 términos, términos de
  más de 80 caracteres o con caracteres de control.
- `404 NOT_FOUND`: el id no existe.
- `200`: `data: { "id": 7, "keywords": [...] }`, con la lista ya normalizada.

`GET /api/v1/interview/:id` incluye `"keywords": [...]` (vacío si no hay). Al procesar, whisper usa el
glosario como contexto y luego un paso de Ollama corrige variantes mal transcriptas (paso
`current_step = "aplicando_glosario"`). Sin palabras clave, ese paso no corre.

### `POST /api/v1/interview/:id/process` — disparar transcripción
`InterviewController::processInterview`. Botón "Enviar a procesar" de la UI:
1. `404 NOT_FOUND` si el id no existe.
2. `409 AUDIO_REQUIRED` si la entrevista todavía no tiene audio asociado.
3. Si tiene audio, marca `status = 'processing'` y responde `202`:
   ```json
   { "success": true, "data": { "id": 7, "status": "processing", "include_summary": false, "enhance_transcript": false }, "error": null }
   ```

Body opcional (sin body o sin un campo, ese campo vale `false`):

```json
{ "include_summary": true, "enhance_transcript": true }
```

- **Por defecto** (ambos `false`): la transcripción entregada es la de whisper con los hablantes
  identificados por diarización (`current_step = "diarizando"`, ADR-021). Esto último aplica si están
  instalados sherpa-onnx y sus modelos. No se llama a Ollama y **no está anonimizada** (ADR-017).
- `enhance_transcript`: corrección turno por turno + anonimización vía Ollama (experimental). No cambia
  los hablantes. Si se pidió y la anonimización falla, la transcripción lleva un aviso `[AVISO HERMES]`.
- `include_summary`: resumen como documento aparte (`GET /interview/:id/summary`). Se genera sobre el texto
  de whisper, o sobre el anonimizado si `enhance_transcript` está activo.

**Importante — límite conocido**: esto solo deja constancia de que se pidió
procesar. No hay cola de trabajos ni Whisper integrado todavía (Sprint 4/5
del roadmap), así que ninguna entrevista llega sola a `completed` o `failed`
— esos dos estados quedan sin forma de alcanzarse hasta que exista esa
integración. `TranscriptionController` (esqueleto en
`backend/api/include/transcriptionController.h`) ya compila pero sigue sin
implementación ni ruta registrada — es de ahí, no de `InterviewController`,
de donde debería salir la transcripción real cuando se construya.

### `GET /api/v1/interview/:id/transcript` — vista de lectura
`InterviewController::getTranscript`, vía `InterviewService::getTranscriptDocument`
y `TranscriptDocumentBuilder`. Devuelve la transcripción final estructurada
para leerla en la app (y "Imprimir / Guardar PDF" desde el navegador), en vez
del `.txt` plano de `/download/transcript`.

```json
{
  "id": 3,
  "has_speakers": true,
  "has_timestamps": true,
  "speaker_source": "diarization",
  "edited": false,
  "notice": null,
  "summary": "texto del resumen o null",
  "speakers": [
    { "key": "interviewer", "label": "Investigador" },
    { "key": "subject", "label": "Monitor GES" }
  ],
  "blocks": [
    { "speaker": "interviewer", "start": 2.88, "end": 14.86, "text": "..." },
    { "speaker": "subject", "start": 16.53, "end": 75.51, "text": "..." }
  ]
}
```

- `speaker` es la **clave del rol** (`interviewer` / `subject` / `null`). El
  nombre visible está en `speakers`: "Investigador" y el `subject_type` de la
  entrevista (ADR-022). `speakers` siempre trae ambos roles.
- `speaker_source`:
  - `diarization`: identificados por el audio (ADR-021);
  - `manual`: versión editada por el usuario;
  - `llm`: etiquetas de IA sobre texto, solo en entrevistas procesadas antes de ADR-022;
  - `none`: sin hablantes.
- Bloques:
  - con hablantes, un bloque por turno, uniendo fragmentos consecutivos del mismo hablante (en la versión editada, un bloque por turno tal como se guardó);
  - sin hablantes, párrafos de 4–9 segmentos;
  - `start`/`end` en segundos, `null` si no se conocen.
- `edited`: hay una versión editada vigente (la original se puede restaurar).
- `notice`: el aviso `[AVISO HERMES] ...` cuando la anonimización pedida
  falló, separado del texto.

Errores: `404 NOT_FOUND` (entrevista inexistente), `409 TRANSCRIPTION_NOT_READY`
(sin resultado), `410 TRANSCRIPTION_FILE_MISSING` (la BD registra un resultado
pero el archivo ya no está en disco).

### `PUT /api/v1/interview/:id/transcript` — guardar la versión editada
`InterviewController::updateTranscript`, vía `InterviewService::updateTranscript`.

Body:
```json
{ "blocks": [ { "speaker": "interviewer", "start": 2.88, "end": 14.86, "text": "..." } ] }
```
`speaker` puede ser `null` (sin asignar); `start`/`end` pueden ser `null`.

- **Guardado:** se guarda en `transcript_edited.json`. La versión del pipeline no se toca.
- **Bloques:** los bloques vacíos se descartan.
- **Aviso:** se conserva el `notice` vigente.
- **Respuesta:** `200` con el mismo payload que el `GET`.
- **Errores:**
  - `400 VALIDATION_ERROR`: body sin `blocks`, bloque sin `text`, hablante desconocido, más de 20 000 bloques, más de 20 000 caracteres en un bloque, caracteres de control, tiempos negativos o `start > end`, o transcripción vacía;
  - `404 NOT_FOUND`;
  - `409 TRANSCRIPTION_NOT_READY`;
  - `409 INTERVIEW_BUSY`: hay un procesamiento en curso;
  - `409 TRANSCRIPTION_NOT_EDITABLE`: entrevista procesada antes de ADR-022, hay que reprocesarla.

### `DELETE /api/v1/interview/:id/transcript/edits` — restaurar el original
Descarta `transcript_edited.json`. Responde `200` con el documento original.
Errores: `404`, `409 INTERVIEW_BUSY`, `409 TRANSCRIPTION_NOT_READY`.

### `GET /api/v1/interview/:id/audio` — audio para el reproductor
`InterviewController::getAudio`, vía `InterviewService::readAudio` (ADR-023).

Sirve el audio normalizado `storage/interviews/<id>/audio.wav` (WAV PCM 16 kHz mono), que es la referencia de tiempo de las marcas `start`/`end` de la transcripción. No pasa por el sobre `{success,data,error}` cuando hay audio: el cuerpo son los bytes del archivo.

- **Sin encabezado `Range`:** `200` con el archivo completo (por streaming), `Accept-Ranges: bytes`.
- **Con `Range: bytes=a-b`, `bytes=a-` o `bytes=-n`:**
  - responde `206 Partial Content` con `Content-Range: bytes a-b/total`;
  - cada respuesta trae como máximo 2 MB (`Config::MAX_AUDIO_RANGE_BYTES`), y el navegador pide el tramo siguiente;
  - si hay varios rangos, solo se usa el primero.
- **Encabezados:** `Content-Type: audio/wav`, `Cache-Control: no-store`.
- **Errores:**
  - `416` con `Content-Range: bytes */total` si el rango empieza después del final;
  - `404 NOT_FOUND` si la entrevista no existe;
  - `404 AUDIO_NOT_AVAILABLE` si todavía no se procesó (el WAV se genera al procesarla).

`GET /api/v1/interview/:id` incluye `"transcript_edited": true|false`.
Reprocesar la entrevista descarta las ediciones.

### `DELETE /api/v1/interview/:id`
`InterviewController::deleteInterview`, vía `InterviewService::removeInterview`:
1. `404 NOT_FOUND` si el id no existe.
2. Si existe, borra la fila de `interviews`. `interviews_audio` e
   `interview_results` se eliminan solos por `ON DELETE CASCADE`
   (`SQL/init.sql`).
3. Si la entrevista tenía audio asociado, además borra el archivo físico en
   `./uploads` (best-effort — si falla, solo se loguea, no aborta la
   respuesta). Necesario por Privacy First: sin este paso, "eliminar" una
   entrevista dejaría el audio real huérfano en disco. Por el mismo motivo
   borra `storage/interviews/<id>` (transcripciones, resumen, audio
   normalizado).
4. Responde `200` con `data: { "id": 7, "code": "DELETED" }`.

```json
{ "success": true, "data": { "id": 7, "code": "DELETED" }, "error": null }
```

---

## 2. Columna `status`

`interviews.status VARCHAR(20) NOT NULL DEFAULT 'pending_audio'`
(`SQL/init.sql`). Valores usados por el frontend
(`src/api/types.ts::InterviewStatus`): `pending_audio` | `pending_processing`
| `processing` | `completed` | `failed`.

La migración es retroactiva: `SQL/init.sql` trae, además del `CREATE TABLE
IF NOT EXISTS`, un `ALTER TABLE ... ADD COLUMN` explícito para bases que ya
existían sin esta columna. MySQL (a diferencia de MariaDB) no soporta `ADD
COLUMN IF NOT EXISTS`, así que la idempotencia la garantiza
`DatabaseManager::migrateTables`, que tolera el error 1060 (`ER_DUP_FIELDNAME`,
"la columna ya existe") en vez de abortar la migración.

**Nota sobre datos viejos**: filas de `interviews` creadas antes de este
cambio (con audio ya subido en su momento) van a mostrar `pending_audio`
hasta que se las reprocese, porque el valor por default se aplicó
retroactivamente sin inspeccionar `interviews_audio`. No afecta datos
nuevos — el ciclo completo (`pending_audio` → `pending_processing` vía
`/upload` → `processing` vía `/interview/:id/process`) ya se probó de punta
a punta.

---

## 3. CORS / mismo origen

El frontend en desarrollo corre en `http://localhost:5173` (Vite) y el
backend en `http://localhost:18080`. El `vite.config.ts` del frontend define
un **proxy**: toda request a `/api/*` se reenvía tal cual (sin reescribir el
path) a `http://localhost:18080/*`. Como el backend ya expone sus rutas bajo
ese mismo prefijo `/api/v1/...`, el path llega intacto. Como el navegador ve
todo como el mismo origen, esto evita CORS por completo en desarrollo — no
requiere ningún cambio en el backend.

Si en el futuro el frontend se sirve compilado (`npm run build`) desde un
origen distinto al backend, ahí sí hace falta uno de estos dos (**todavía
sin implementar, no bloquea nada en desarrollo**):
- Servir frontend y backend detrás del mismo reverse proxy (mismo origen), o
- Agregar en Crow los headers `Access-Control-Allow-Origin`,
  `Access-Control-Allow-Methods`, `Access-Control-Allow-Headers` (incluyendo
  `interview_id`, que es un header custom y dispara preflight `OPTIONS`), y
  manejar el método `OPTIONS` explícitamente (hoy cae en el
  `CROW_CATCHALL_ROUTE` y devuelve un 404 JSON, no una respuesta de
  preflight válida).

---

## 4. Estado actual

| # | Endpoint / cambio | Estado |
|---|--------------------|--------|
| 1 | `POST /interview` devuelve `id` | ✅ |
| 2 | `GET /interviews` (listado) | ✅ |
| 3 | `GET /interview/:id` (detalle) | ✅ |
| 4 | `POST /interview/:id/process` | ✅ (solo marca `processing`, sin pipeline real) |
| 5 | Columna `status` en `interviews` | ✅ (retroactivo vía migración idempotente) |
| 6 | `transcriptionController.h` compila | ✅ (sin implementación ni ruta — Sprint 5) |
| 7 | CORS | pendiente, solo si se despliega fuera del proxy de Vite |
| 8 | `DELETE /interview/:id` | ✅ (borra fila + cascada + archivo de audio en disco) |
| 9 | Controllers sin acceso directo a `DatabaseManager` | ✅ (`IInterviewRepository` + `InterviewService`, ADR-012) |
| 10 | Versionado de API (`/api/v1` prefix) | ✅ (ADR-013; frontend y proxy de Vite actualizados en el mismo cambio) |

El flujo completo — crear entrevista, ver el id, subir audio, ver el detalle
con el audio asociado, disparar "procesar", eliminarla y ver el estado
cambiar — funciona de punta a punta contra el backend real. Lo que falta
para "terminar" el producto es la transcripción real (Whisper) y sus
estados `completed`/`failed`, que son trabajo de otro sprint, no de la API
en sí.
