---
title: Estado Actual del Proyecto
aliases: ["Current Status", "Session Handoff", "Dónde quedamos"]
tags: [project, hermes, status, session-log]
status: stable
created: 2026-08-15
updated: 2026-10-02
source: Sesiones de trabajo 2026-08-15 y 2026-10-02 (Claude Code)
related: ["Roadmap General de Hermes", "Ollama Integration Strategy", "GPU Acceleration Strategy", "Hermes - Vision General"]
---

# Summary

Foto del estado real del proyecto al 2026-10-02, pensada para retomar el trabajo sin perder contexto — incluyendo al transferir la sesión a otra máquina. Es la nota que hay que leer primero; el detalle sprint por sprint vive en [[Roadmap General de Hermes]].

# Explanation

## Dónde está el código ahora mismo (2026-10-02)

**Nada de lo hecho desde el 2026-08-15 está commiteado.** El último commit es `196483f` (documentación de la bóveda, v0.1.0). En el árbol de trabajo hay ~70 archivos modificados o nuevos que incluyen las sesiones intermedias (video .mp4, VAD, ADR-016/017/018, glosario, validación de la entrevista 13 en `docs/VALIDACIONES/`) y la sesión de hoy. Antes de seguir conviene commitear por bloques.

El usuario hizo una **ejecución limpia** el 2026-10-02: borró `backend/storage/` a mano. Las entrevistas que la base marca como `completed` (p. ej. la 3) ya no tienen archivos; la vista de lectura responde `410 TRANSCRIPTION_FILE_MISSING` hasta reprocesarlas.

## Qué se hizo en la sesión 2026-10-02

Detalle completo en `changelog/2026-10-02_rediseno-frontend-y-vista-de-lectura.md`.

1. **Rediseño visual del frontend** ([[Sprint 8 - Frontend]]) con identidad griega clásica, a partir de dos mockups en `docs/img/mockups/`. Se escribió antes un prompt de contexto para la IA de mockups (no quedó en archivo, solo en el chat).
2. **Reconexión automática a MySQL** ([[Sprint 2 - Persistence]]): un reinicio de MySQL dejaba el backend inutilizable.
3. **Reemplazo de audio sin huérfanos** ([[Sprint 3 - File Upload]]): validación previa 404/409, reemplazo con borrado del resultado anterior, limpieza de 12 archivos huérfanos (~535 MB).
4. **Vista de lectura de la transcripción** con "Imprimir / Guardar PDF" desde el navegador (ADR-019): endpoint `GET /interview/:id/transcript`, `TranscriptDocumentBuilder`, página `/interviews/:id/transcript` con panel de scroll y búsqueda.

## Qué falta hacer, en orden sugerido

1. **Commitear** el árbol de trabajo por bloques (sesiones intermedias + esta).
2. **Probar con datos reales** la vista de lectura (una entrevista con "Corregir y anonimizar" y otra sin) y el reemplazo de audio, sobre el `storage/` limpio.
3. **`executeQuery` silencia errores**: devuelve vacío ante un fallo, así `GET /interviews` responde `[]` con éxito. Toca todos los repositorios.
4. **Precisión y recall de anonimización** ([[Ollama Integration Strategy]]) — sigue bloqueando [[Sprint 7 - Export]] y recomendar `enhance_transcript`.
5. **Glosario**: algunos bloques cortan en `num_predict=512` y quedan sin sanitizar (visto en `API.log`, entrevista 3).
6. **Opciones del job en la API**: hoy no se persisten (requiere migración); el frontend las recuerda en el navegador. Exponerlas permitiría mostrar el recorrido exacto y el estado "completado sin anonimizar".
7. Revisar visualmente el **modo claro**.
8. GPU (Vulkan) — ver [[GPU Acceleration Strategy]].

## Hallazgos abiertos, no bugs a resolver todavía

- Etiqueta `Investervistado:` (interview_id=10, encontrada 2026-08-15): a distancia de edición 6 de ambas etiquetas canónicas, empatada — se decidió **no** normalizarla automáticamente porque adivinar cuál de los dos hablantes es sería un coinflip silencioso. Ver [[Ollama Integration Strategy]], Common Mistakes.
- La vista de lectura muestra esas variantes no canónicas como texto del turno anterior (no las descarta).

## Sesiones anteriores

### 2026-08-15
#### Dónde estaba el código

El código (fixes de C++, `README.md`, `CHANGELOG.md`, versión de CMake) ya quedó commiteado en `738ce00 "OllamaClient::chat()"` (2026-08-15). Esta actualización de la bóveda de conocimiento se commitea aparte, inmediatamente después de escribir esta nota. Si estás retomando desde otra máquina, `git log` es la fuente de verdad — si no ves un commit de documentación de la bóveda posterior a `738ce00`, es que ese commit no llegó a hacerse.

**Nota de contexto sobre la transferencia de máquina**: el mismo commit `738ce00` incluye 3 cambios de red que no salieron de esta sesión de documentación (probablemente hechos a mano para preparar la transferencia): `backend/main.cpp` ahora hace bind a `0.0.0.0` en vez de solo loopback, `frontend/package.json` corre `vite --host`, y `frontend/vite.config.ts` apunta el proxy a `http://192.168.100.6:18080` (IP LAN, no `127.0.0.1`). Esa IP es específica de la máquina de origen — si el backend corre en una IP LAN distinta en la máquina nueva, `vite.config.ts` necesita actualizarse a mano, si no el proxy del frontend no va a encontrar el backend.

#### Qué se hizo

1. **Backlog de calidad de Ollama, items 1 y 2** (ver [[Ollama Integration Strategy]]): determinismo entre corridas (`temperature=0`, `seed` fijo) y normalización de variantes de etiqueta de hablante (`Investigado:`/`Entrevistador:` → forma canónica, por distancia de edición ≤3).
2. **Bug de encoding encontrado y arreglado**: descargas de transcript/resumen mostraban acentos rotos en algunos editores de Windows. El archivo en disco y la respuesta HTTP siempre estuvieron en UTF-8 correcto — faltaba el BOM, sin el cual algunos editores adivinan mal la codificación. Arreglado agregando el BOM solo en la respuesta de descarga (`interviewController.cpp`), no en el archivo guardado.
3. **Investigación de "el transcript se salta partes de la entrevista"** (reportado por el usuario comparando audio vs. texto): se verificó que, para las dos entrevistas de prueba analizadas, whisper.cpp cubre la práctica totalidad del audio normalizado sin huecos internos, y que el paso de Ollama no pierde contenido respecto al raw de whisper (solo fusiona fragmentos en oraciones por hablante). **Sin resolver** — no se identificó la causa raíz; queda pendiente un timestamp puntual del usuario para poder investigar un caso concreto. Ver [[Ollama Integration Strategy]] y la sección "Próximos pasos" abajo.
4. **Primera pre-release, v0.1.0** (pre-release / early preview, no v1.0): se decidió explícitamente con el usuario no llamarla "1.0 stable" porque el proyecto no cumple ese alcance todavía (sin Export, sin Configuration, sin tests, sin instalador, y con el recall de anonimización incompleto). Se preparó `README.md` (reescrito para explicar el proyecto tal como es hoy, con secciones "Qué falta para v1.0" y "Limitaciones conocidas") y `CHANGELOG.md` nuevo.
5. **Esta actualización de la bóveda de conocimiento** — reconciliar `hermes-vault-knowledge/04 Roadmap/` con `.ai/ROADMAP.md` (que ya no está vacío, a diferencia de cuando se escribió la nota original de [[Roadmap General de Hermes]]) y crear esta nota.

#### Qué faltaba (al 2026-08-15)

1. **Decidir y ejecutar el tag/release real**: ¿`git tag v0.1.0` + push + GitHub Release? Todavía no se hizo, solo se preparó el contenido (README, CHANGELOG, versión de CMake).
2. **Recall de anonimización** (backlog de [[Ollama Integration Strategy]], item 4) — el más importante, bloquea [[Sprint 7 - Export]]. Necesita su propia sesión: prompt con ejemplos few-shot, posible lista determinística de nombres conocidos como refuerzo, y varias corridas de prueba para medir mejora real.
3. **Investigar el reporte de "audio saltado"** — pedirle al usuario un timestamp puntual de una entrevista real donde note contenido faltante, para poder comparar ese tramo exacto contra `transcript_raw.json` y whisper.cpp con datos concretos en vez de inferencia.
4. **Validación de idioma/alfabeto** (backlog item 3) — evita que alucinaciones de whisper en otro idioma pasen sin corregir.
5. GPU (Vulkan para whisper.cpp) — ver [[GPU Acceleration Strategy]], investigado y planeado, no implementado. Palanca de rendimiento, no de calidad — menor prioridad que lo de arriba.


# Why it matters

Hermes viene de un ritmo de sesiones intensas con hallazgos reales sobre datos de producción (ver [[Ollama Integration Strategy]], [[GPU Acceleration Strategy]]) que quedaban documentados pero dispersos entre `.ai/ROADMAP.md` y notas individuales. Esta nota es el punto de entrada único para no perder el hilo entre sesiones o al cambiar de máquina — el resto de la bóveda tiene el detalle, esta nota tiene el resumen y el "y ahora qué".

# Best Practices

- Actualizar esta nota al cierre de cada sesión de trabajo relevante, no solo cuando se transfiere de máquina.
- Si esta nota contradice `.ai/ROADMAP.md` o el estado real del `git log`, confiar en `git log` y corregir esta nota — no al revés.
- Mantenerla corta y accionable (qué se hizo, qué falta, en qué orden) — el detalle técnico va en la nota específica (Sprint, ADR, o nota de arquitectura) enlazada desde acá.

# Common Mistakes

- Asumir que algo descrito acá ya está commiteado/pusheado sin verificar con `git log`/`git status` — ver la advertencia al principio de esta sesión en particular.
- Dejar esta nota desactualizada mientras se actualiza `.ai/ROADMAP.md` (o viceversa) — ambas deben quedar consistentes al cierre de sesión.

# Hermes Usage

Primera nota a leer al retomar el trabajo en Hermes, antes que [[Roadmap General de Hermes]] o cualquier nota de sprint individual.

# Related Notes

- [[Roadmap General de Hermes]]
- [[Ollama Integration Strategy]]
- [[GPU Acceleration Strategy]]
- [[Hermes - Vision General]]

# References

- Sesiones de trabajo 2026-08-15 y 2026-10-02 (Claude Code) — este documento
- `changelog/` de esta bóveda — un archivo por sesión
- `.ai/ROADMAP.md` (Priority 1)
