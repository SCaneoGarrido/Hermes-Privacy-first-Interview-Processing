# Changelog

Formato basado en [Keep a Changelog](https://keepachangelog.com/), versionado según [Semantic Versioning](https://semver.org/).

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
