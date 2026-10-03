# Changelog

Formato basado en [Keep a Changelog](https://keepachangelog.com/), versionado según [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Carga de video `.mp4`: se procesa solo su pista de audio y el video original se borra tras extraerla (ADR-016).
- Tope de subida de 1 GB (`413 PAYLOAD_TOO_LARGE`), validado también en el frontend antes de transferir.

- VAD (Silero) opcional en la transcripción (`WHISPER_VAD_MODEL_PATH`).
- Opción `enhance_transcript` al procesar (casilla "Corregir y anonimizar con IA" en el frontend).
- Palabras clave por entrevista (ADR-018).
  - Se cargan desde un `.txt` en el detalle de la entrevista y se guardan con `PUT /interview/:id/keywords`.
  - Guían a whisper (`initial_prompt`) y alimentan un paso de sanitización con Ollama que solo corrige variantes de esos términos.
  - Los cambios aplicados quedan en `glossary_changes.txt`.
- Vista de lectura de la transcripción en la app (ADR-019): portada con la ficha y el aviso de revisión, resumen, turnos por hablante o párrafos con marca de tiempo, búsqueda y marcadores de anonimización resaltados. "Imprimir / Guardar PDF" desde el navegador. Nuevo endpoint `GET /api/v1/interview/:id/transcript`.
- Reemplazo del audio de una entrevista: borra el audio anterior y el resultado previo, con confirmación en el frontend.

### Changed

- Rediseño visual del frontend con identidad griega clásica (paleta de cerámica ática, títulos epigráficos, meandro, íconos y logo propios). Lista con contadores, búsqueda y filtros; detalle organizado por estado con recorrido de pasos del procesamiento. Sin dependencias nuevas.
- `POST /upload` valida antes de escribir el archivo: `404` si la entrevista no existe, `409 INTERVIEW_BUSY` si se está procesando.
- **La corrección + anonimización con IA pasa a ser opcional y está apagada por defecto** (ADR-017). La transcripción entregada por defecto es la salida plana de whisper (una línea por segmento, sin anonimizar), y sin opciones activadas no se llama a Ollama.
- El resumen sigue siendo un documento aparte. Si se pide sin corrección, se genera sobre el texto de whisper.

### Fixed

- El backend no se reconectaba a MySQL: tras un reinicio de MySQL respondía todo con "Server has gone away" hasta reiniciarlo. Ahora reconecta solo.
- Subir un segundo audio a una entrevista fallaba con `Duplicate entry` y el archivo quedaba huérfano en `./uploads`. Ningún upload rechazado queda ya en disco.
- Eliminar una entrevista dejaba sus transcripciones en `storage/interviews/<id>`; ahora se borran.
- El tamaño del archivo subido se guardaba en un `int` (desborde por encima de 2 GB); ahora es `long long`.
- whisper.cpp podía entrar en un bucle repitiendo la misma frase hasta el final del audio (entrevista 13: ~7 min perdidos). Ahora el contexto de texto previo está acotado, y los tramos en bucle se detectan y se re-transcriben sin contexto y con beam search. Si no se recuperan, se marcan como `[audio no transcrito mm:ss-mm:ss]`.
- Ollama truncaba en silencio los bloques largos (contexto por defecto de 4096 tokens). Ahora se fijan `num_ctx=8192` y un tope `num_predict` contra la generación sin fin.
- Una falla en la anonimización descartaba la corrección ya hecha. Ahora se conserva (`transcript_corrected.txt`), y si se pidió anonimizar y falló, la transcripción entregada lo indica con un aviso al inicio del archivo.

## [0.1.0] - 2026-08-15

Primera pre-release / early preview. Pipeline end-to-end funcional y verificado contra entrevistas reales, pero **no** cubre todavía el alcance completo planeado para v1.0 (Export, Configuration, Testing, instalador). Ver README.md ("Qué falta para v1.0" y "Limitaciones conocidas") antes de usar con datos sensibles reales.

### Added

- API REST (`/api/v1`) para gestión de entrevistas: crear, listar, consultar, eliminar.
- Carga de audio con validación por firma de bytes.
- Procesamiento en background: cola de jobs con estados (`pending`/`running`/`completed`/`failed`), reintentos, recuperación de jobs interrumpidos al reiniciar el backend.
- Normalización de audio local vía FFmpeg (mono, 16kHz, PCM16).
- Transcripción local con whisper.cpp.
- Corrección ortográfica/de puntuación y estructuración por hablante vía Ollama (LLM local).
- Anonimización de nombres, lugares y organizaciones (tabla de sustitución consistente en toda la entrevista).
- Resumen final opcional (`include_summary`).
- Frontend en React: carga de entrevistas, listado, detalle, seguimiento de progreso, descarga de resultados.

### Fixed

- Descargas de transcripción/resumen mostraban acentos rotos (mojibake) en algunos editores de texto de Windows por falta de BOM UTF-8 en la respuesta de descarga.
- Atribución de hablante no determinista entre corridas del mismo audio — ahora se fija `temperature=0` y `seed` en las llamadas a Ollama.
- Variantes de etiqueta de hablante inventadas por el modelo (ej. `Investigado:`, `Entrevistador:`) ahora se normalizan a `Investigador:`/`Entrevistado:` cuando están lo bastante cerca de la forma canónica.

### Known limitations

- El recall de la anonimización automática es incompleto — no confiar en ella sin revisión humana.
- La atribución de hablante es una aproximación heurística sobre texto, no diarización acústica real.
- whisper.cpp puede alucinar texto en otro idioma en tramos de audio poco claros.
- Todo el pipeline corre en CPU; no hay aceleración por GPU todavía.
- Sin tests automatizados — solo verificación manual contra entrevistas reales.
