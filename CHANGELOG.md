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
- **Identificación de hablantes por la voz** (diarización acústica local con sherpa-onnx, ADR-021). **En pruebas.**
  - Nuevo paso `diarizando`.
  - Cada turno se etiqueta como "Investigador" o con el tipo de sujeto de la entrevista (ej. "Monitor GES").
  - Agrupación por umbral y reducción a dos roles: el grupo que más pregunta es el investigador.
  - Validada en dos entrevistas reales. En audio con voces muy parecidas o diálogo muy rápido puede no separar, y entonces entrega la transcripción sin hablantes.
  - Opcional: sin la DLL y los modelos se transcribe igual, sin hablantes.
- Capturas de la interfaz en el README.
- **Reproducción sincronizada en la vista de lectura** (ADR-023).
  - Reproductor que resalta el turno que se está escuchando, con seguimiento automático, en lectura y en edición.
  - Clic en una marca de tiempo para saltar; ±5 s, velocidad y atajos Alt+K / Alt+J / Alt+L.
  - Nuevo `GET /interview/:id/audio` con soporte de `Range` (206), implementado a mano porque Crow no lo trae.
- **Aceleración por GPU (Vulkan)** para whisper.cpp, con caída automática a CPU (`WHISPER_USE_GPU`, ADR-020).
- **Edición de la transcripción en la vista de lectura** (ADR-022).
  - Texto y hablante por turno; dividir, unir y eliminar turnos; intercambiar los dos hablantes.
  - Nuevos `PUT /interview/:id/transcript` y `DELETE /interview/:id/transcript/edits`.
  - La versión original se conserva y se puede restaurar. Reprocesar descarta las ediciones, con aviso previo.

### Changed

- **Mayor fidelidad de transcripción por defecto** (ADR-020):
  - modelo `ggml-large-v3`, beam search en GPU;
  - VAD ajustado para no recortar palabras ni partir frases;
  - se descartan tokens de no-habla ("[Música]") y alucinaciones conocidas de subtítulos.
- La transcripción entregada se guarda estructurada (`transcript_segments.json`) con hablante y tiempos en ms por turno. El `.txt` y la vista de lectura se generan a partir de ella. La descarga refleja las ediciones y el nombre de sujeto actual (ADR-022).
- La corrección con IA (`enhance_transcript`) ya no asigna hablantes. Corrige turno por turno y, si el modelo omite o altera demasiado una línea, conserva el texto original: deja de perderse contenido en los cortes de bloque.

- Rediseño visual del frontend con identidad griega clásica (paleta de cerámica ática, títulos epigráficos, meandro, íconos y logo propios). Lista con contadores, búsqueda y filtros; detalle organizado por estado con recorrido de pasos del procesamiento. Sin dependencias nuevas.
- `POST /upload` valida antes de escribir el archivo: `404` si la entrevista no existe, `409 INTERVIEW_BUSY` si se está procesando.
- **La corrección + anonimización con IA pasa a ser opcional y está apagada por defecto** (ADR-017). La transcripción entregada por defecto es la salida plana de whisper (una línea por segmento, sin anonimizar), y sin opciones activadas no se llama a Ollama.
- El resumen sigue siendo un documento aparte. Si se pide sin corrección, se genera sobre el texto de whisper.

### Fixed

- **Frase de estilo filtrada y texto cortado.**
  - En audio difícil, whisper copiaba en la transcripción la frase de estilo del prompt ("Transcripción fiel de una entrevista, con puntuación…") y descartaba ventanas enteras de 20-30 s con habla.
  - Se reemplazó por una línea de diálogo neutra ("¿Y cómo lo hacen ustedes? Bueno, depende del caso…").
  - Se descartan los ecos del prompt y los créditos de subtítulos alucinados ("Transcripción y subtítulos por…").
  - Se re-transcriben los huecos y los tramos largos sin puntuar.
  - Entrevista 2: 0 frases filtradas (antes 5), 0 s de habla perdida (antes 21 s), 48 de 49 minutos puntuados.
- El filtro de densidad ya no borra habla real con marcas de tiempo rotas: solo descarta segmentos densos que repiten un segmento cercano.
- La vista de lectura nunca mostraba marcas de tiempo: los inicios de segmento se leían como número desde un texto `"00:00:02"` y la lectura fallaba siempre.
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
