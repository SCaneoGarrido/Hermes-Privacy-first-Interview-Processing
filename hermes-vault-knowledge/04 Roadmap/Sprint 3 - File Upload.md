---
title: Sprint 3 - File Upload
aliases: []
tags: [roadmap, sprint, hermes]
status: in-progress
created: 2026-07-24
updated: 2026-07-28
source: README.md
related: ["Sprint 2 - Persistence", "Sprint 4 - Background Processing", "ADR-011 - Contrato de Respuesta API Uniforme"]
---

# Summary

Carga de audios de entrevistas mediante upload multipart.

# Explanation

**Objetivo:** carga de audios.

**Entregables (2026-07-28):**
- ✅ Upload multipart (`POST /upload`, campo `file`, header `interview_id`)
- ✅ Validación de formato — en dos capas: `Content-Type` declarado contra una lista permitida (mp3/wav/ogg/m4a), y validación de la **firma real de bytes** del archivo (`AudioSignature.h`: `RIFF...WAVE` para WAV, `OggS` para OGG, box `ftyp` para M4A, tag `ID3`/frame-sync MPEG para MP3). La segunda capa se agregó tras confirmar que el `Content-Type` es controlado por el cliente y trivialmente falsificable (reproducido con Postman: texto plano con `Content-Type: audio/wav` pasaba la validación).
- ✅ Organización de archivos (`./uploads`, `Config::UPLOAD_DIRECTORY`)
- ✅ Identificadores únicos (UUID vía `boost::uuid`, nombre del archivo guardado)
- ❌ Progreso de subida (no aplica a un upload síncrono simple; queda para cuando exista cola de trabajos)

# Why it matters

Es el punto de entrada de datos sensibles al sistema; su validación es la primera línea de defensa antes de que el audio llegue a whisper.cpp. La lección concreta de este sprint: **el `Content-Type` de un multipart no prueba nada por sí solo** — solo la firma de bytes del archivo lo hace.

# Best Practices

- Validar tipo y tamaño de archivo en el backend, no solo en el cliente React ([[API First]]).
- No confiar en el `Content-Type` ni en la extensión declarados por el cliente: inspeccionar los bytes reales del archivo.

# Common Mistakes

- Confiar únicamente en la extensión o el `Content-Type` del archivo para determinar su formato real (ver `AudioSignature.h`).

# Hermes Usage

`FileFormatGuard` (middleware) hace las tres validaciones (cuerpo no vacío, `Content-Type` permitido, firma de bytes) antes de que la request llegue al controller. Al guardar el audio, `FileController` también actualiza el `status` de la entrevista a `pending_processing` (ver [[Sprint 2 - Persistence]]). Prepara el terreno para [[Sprint 4 - Background Processing]] y [[Sprint 5 - Whisper Integration]].

# Related Notes

- [[Sprint 2 - Persistence]]
- [[Sprint 4 - Background Processing]]
- [[ADR-011 - Contrato de Respuesta API Uniforme]]

# References

- README.md (Priority 2)
- docs/API_REQUIREMENTS.md (Priority 1)
