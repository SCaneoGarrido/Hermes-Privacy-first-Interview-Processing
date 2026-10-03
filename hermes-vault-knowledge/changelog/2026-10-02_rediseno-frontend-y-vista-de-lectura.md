# Changelog

**Fecha:** 2026-10-02

## Resumen

Sesión centrada en el frontend y en la robustez del manejo de datos:

- Rediseño visual completo con identidad griega clásica, a partir de dos mockups.
- Reconexión automática a MySQL.
- Reemplazo de audio sin dejar archivos huérfanos en disco.
- Vista de lectura de la transcripción, con impresión a PDF desde el navegador.

## Cambios realizados

### Diseño y mockups
- Se escribió un prompt de contexto detallado para una IA de mockups. Define la identidad griega (Hermes mensajero, guía de umbrales, "hermético"), las restricciones de la API, lo que está fuera de alcance y las reglas de honestidad sobre privacidad. Solo quedó en el chat, no en un archivo.
- La IA generó dos mockups, guardados en `docs/img/mockups/` (lista de entrevistas y detalle en "Procesando").
- Se revisaron contra la API real. Se descartaron los elementos sin dato detrás:
  - métricas inventadas (SHA-256, kHz, PID, hilos AVX2, puerto 8000);
  - "Cancelar procesamiento", que no tiene endpoint;
  - consola de CPU;
  - avatar de usuario;
  - pestañas de estados del mockup.
- También se descartaron afirmaciones falsas: "aislamiento criptográfico", "procesamiento en memoria RAM" y "100 % seguridad".

### Frontend: rediseño (Sprint 8)
- **Identidad visual:**
  - Tokens nuevos en `styles.css`: negro ático, terracota, marfil, azul Egeo y oro viejo, en modo oscuro y claro.
  - Títulos epigráficos con serif del sistema (Palatino Linotype/Georgia).
  - Meandro como divisor (máscara SVG inline).
  - Isotipo "H" con ala (`HermesMark`), íconos SVG propios (`Icons.tsx`) y favicon inline.
  - Sin dependencias nuevas ni recursos de red.
- **Lista ("Corpus de entrevistas"):**
  - Contadores (total, en proceso, completadas).
  - Búsqueda, orden y filtros por estado en el cliente.
  - Tarjetas con borde por estado y una acción principal por estado.
  - `ConfirmDialog` propio para borrar.
  - Refresco automático cada 8 s mientras hay algo procesándose; pide el detalle solo de esas filas para mostrar el paso.
- **Detalle:**
  - Layout según el estado y ficha de la entrevista.
  - "Secuencia de umbrales" (`ThresholdStepper`, numerales I–VI).
  - `Dropzone` con validación de extensión y tamaño.
  - Cronómetro rotulado "Tiempo observado": la API no da el inicio del job.
  - Avisos de revisión junto a las descargas y tarjeta de error con "Reintentar".
- **Nueva entrevista:** indicador de pasos I Datos → II Audio → III Procesar.
- **Opciones del procesamiento:** la API no devuelve con qué opciones se lanzó un job. `processingSteps.ts` las recuerda en `localStorage` para los jobs lanzados desde ese navegador; si no las conoce, muestra los pasos opcionales como "Si se pidió".
- **Helpers nuevos:** `format.ts` (fechas, tamaños, duraciones, numerales romanos) y `processingSteps.ts`.
- **ErrorBanner:** muestra títulos legibles según el código de error. Se quitó el texto viejo que decía "endpoint no implementado" ante cualquier 404.

### Backend: reconexión a MySQL (Sprint 2)
- **Diagnóstico** (desde `API.log` y Docker): MySQL se había reiniciado y el backend seguía usando la conexión muerta, que nunca reabría. Además había dos `backend.exe` corriendo a la vez.
- **Arreglo:** `DatabaseManager::ensureConnected()` hace `mysql_ping` antes de cada `executePrepared`/`executeQuery`, con el mutex tomado, y reconecta con los parámetros guardados en memoria. Verificado por el usuario.

### Backend y frontend: reemplazo de audio (Sprint 3)
- **Diagnóstico:**
  - Subir un segundo audio fallaba con `Duplicate entry` (UNIQUE `interview_id`).
  - El archivo se guardaba antes del INSERT, así que quedaba huérfano: 12 de 13 archivos en `uploads/`, unos 535 MB. Se borraron con aprobación del usuario, previa verificación contra la base.
- **Validación previa:** `InterviewService::canAttachAudio` → `404 NOT_FOUND` / `409 INTERVIEW_BUSY`, antes de escribir el archivo.
- **Reemplazo:** `InterviewService::attachAudio` (ahora devuelve `AttachAudioOutcome`):
  - si ya había audio, actualiza la fila;
  - borra el archivo anterior;
  - borra el resultado previo: `IInterviewRepository::removeResults` y `storage/interviews/<id>`;
  - borra el upload si lo rechaza.
- **Eliminar una entrevista** ahora también borra `storage/interviews/<id>`.
- **Frontend:** confirmación antes de reemplazar y mensajes de error claros.

### Vista de lectura (ADR-019)
- **Backend:**
  - `TranscriptDocument` y `TranscriptDocumentBuilder`, puros y probados con un programa aparte: turnos por hablante, aviso de anonimización, texto plano con marcas de tiempo con y sin coincidencia de segmentos.
  - `InterviewService::getTranscriptDocument`, que lee `transcript_final.txt`, `transcript_raw.json` y el resumen.
  - `GET /api/v1/interview/:id/transcript`, con errores 404/409/410.
- **Frontend** (`TranscriptReaderPage`, ruta `/interviews/:id/transcript`):
  - Portada con la ficha y un aviso de revisión que cambia según haya marcadores o hablantes.
  - Resumen.
  - Transcripción en un panel con scroll propio.
  - Búsqueda con contador y salto a la primera coincidencia.
  - Marcadores `[PERSONA_1]` resaltados.
- **PDF:** botón "Imprimir / Guardar PDF" con hoja `@media print`: A4, portada en hoja propia, número de página y scroll desactivado. Verificado generando el PDF con Edge headless.
- **Accesos:** "Leer transcripción" en el detalle y "Leer" en la lista.

## Archivos modificados

**Backend**
- `backend/api/include/DatabaseManager.h`, `backend/api/shared/database/DatabaseManager.cpp`
- `backend/api/include/repositories/IInterviewRepository.h`, `MySqlInterviewRepository.h`, `backend/api/shared/database/MySqlInterviewRepository.cpp`
- `backend/api/include/services/InterviewService.h`, `backend/api/services/InterviewService.cpp`
- `backend/api/include/services/TranscriptDocument.h` (nuevo), `TranscriptDocumentBuilder.h` (nuevo), `backend/api/services/TranscriptDocumentBuilder.cpp` (nuevo)
- `backend/api/controllers/fileController.cpp`, `backend/api/controllers/interviewController.cpp`, `backend/api/include/interviewController.h`
- `backend/main.cpp`, `backend/CMakeLists.txt`

**Frontend**
- `frontend/index.html`, `frontend/src/styles.css`, `frontend/src/App.tsx`
- `frontend/src/format.ts` (nuevo), `frontend/src/processingSteps.ts` (nuevo)
- `frontend/src/api/types.ts`, `frontend/src/api/interviews.ts`
- `frontend/src/components/`: `Layout.tsx`, `StatusBadge.tsx`, `ErrorBanner.tsx`, `KeywordsCard.tsx`; nuevos: `Icons.tsx`, `HermesMark.tsx`, `ConfirmDialog.tsx`, `Dropzone.tsx`, `ThresholdStepper.tsx`
- `frontend/src/pages/`: `InterviewsListPage.tsx`, `InterviewDetailPage.tsx`, `NewInterviewPage.tsx`; nuevo: `TranscriptReaderPage.tsx`

**Documentación**
- `.ai/DECISIONS.md` (ADR-019), `.ai/ROADMAP.md` (Sprints 2, 3, 7, 8)
- `docs/API_REQUIREMENTS.md` (upload 404/409/reemplazo, `GET /transcript`, borrado de `storage/`)
- `README.md`, `CHANGELOG.md`
- Bóveda: `01 Project/Estado Actual del Proyecto.md`, `04 Roadmap/Roadmap General de Hermes.md`, `Sprint 2 - Persistence.md`, `Sprint 3 - File Upload.md`, `Sprint 8 - Frontend.md`, `Home.md`, este archivo

## Motivo de los cambios

- **Diseño:** el frontend era funcional pero genérico, y el usuario quería una identidad que le hiciera honor al nombre del proyecto.
- **MySQL y audio:** la reconexión y el reemplazo de audio salieron de fallas reales vistas en `API.log`. La segunda dejaba **audio sensible huérfano en disco**, contra el principio de privacidad del proyecto.
- **Vista de lectura:** el `.txt` plano, con una línea por segmento de whisper, es muy difícil de leer. Se eligió imprimir desde el navegador para no sumar una librería de PDF en C++ ni adelantar Export, que sigue bloqueado por la anonimización.

## Estado actual

- **Compilación:** backend y frontend compilan (`cmake --build backend/build --target backend`, `npm run build`).
- **Verificado por el usuario:** reconexión a MySQL y reemplazo de audio. El log muestra "Audio reemplazado; se descartó el resultado anterior" para la entrevista 4.
- **Verificado solo con un mock de la API:** el rediseño y la vista de lectura, en escritorio, a 390 px y en PDF.
- **Ejecución limpia:** el usuario borró `backend/storage/` a mano. La entrevista 3 figura como `completed` pero sin archivos, así que la vista de lectura responde 410 hasta reprocesarla.
- **Nada commiteado** desde `196483f`.

## Próximos pasos

- Commitear el árbol de trabajo por bloques (sesiones intermedias + esta).
- Probar con datos reales la vista de lectura, con y sin "Corregir y anonimizar", y el reemplazo de audio.
- Hacer que `executeQuery` informe los errores en vez de devolver vacío. Hoy `GET /interviews` responde `[]` con éxito ante un fallo de la base.
- Anonimización: precisión y recall. Sigue bloqueando Sprint 7 y recomendar `enhance_transcript`.
- Glosario: bloques cortados en `num_predict=512` quedan sin sanitizar.
- Persistir y exponer las opciones del job (`include_summary`, `enhance_transcript`) y un indicador de anonimización fallida, para no depender de `localStorage`.
- Revisar visualmente el modo claro.

## Contexto para la siguiente sesión

**Decisiones tomadas**
- Diseño: identidad griega con serif del sistema y SVG propios, sin `@fontsource` ni `lucide-react`, para no sumar dependencias. Se evita el caduceo por su confusión con el símbolo médico.
- Opciones del job: se resolvieron solo en el frontend (variante 1b). Exponerlas en la API requiere migrar la base, porque `interview_jobs` no las guarda.
- Reemplazo de audio: borra la transcripción y el resumen anteriores (opción "a", elegida por el usuario).
- PDF desde el navegador y no desde el backend (ADR-019). DOCX para NVivo/Atlas.ti queda para Sprint 7.

**Supuestos importantes**
- La marca de tiempo por párrafo solo aparece si `transcript_final.txt` tiene exactamente una línea por segmento de `transcript_raw.json`. Se cumple con la salida plana de whisper, aun con glosario, porque el glosario reemplaza palabras y no líneas. No se cumple con la salida corregida por IA, que tampoco lleva marcas de tiempo.
- `TranscriptDocumentBuilder` detecta hablantes si al menos la mitad de las líneas empiezan con `Investigador:` o `Entrevistado:`.

**Problemas pendientes**
- Los listados anteriores.
- `vite.config.ts` apunta el proxy a `192.168.100.6:18080`, una IP de LAN de esta máquina.

**Restricciones existentes**
- Sin tests automatizados (no hay Catch2). Verificar a mano.
- Nada de recursos de red en el frontend (offline).
- Ningún texto de la UI puede prometer anonimización completa.

**Consideraciones técnicas**
- `backend.exe` en ejecución bloquea el enlazado: hay que cerrarlo antes de recompilar.
- Para revisar la UI sin backend sirvió un mock de la API (`node`) junto a Vite levantado por su API JS con `configFile: false`, y capturas con `msedge --headless --screenshot` o `--print-to-pdf`. Edge headless no baja de unos 500 px de ancho, así que para ver mobile hay que usar un iframe de 390 px servido por HTTP.
