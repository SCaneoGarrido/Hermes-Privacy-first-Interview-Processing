# Hermes — Guía para Claude Code

Plataforma open source de procesamiento de entrevistas de investigación cualitativa,
**100% local**: transcripción, corrección/diarización por hablante, anonimización de PII
y resumen opcional. Sin cloud, sin APIs externas. Ver `README.md` para el detalle funcional.

## Comandos

Backend (C++20, requiere `VCPKG_ROOT` y MinGW-w64 en el `PATH`):
```
cmake --build backend/build --target backend -j4
```
Frontend:
```
cd frontend && npm run dev       # dev server (Vite)
cd frontend && npm run build     # tsc -b && vite build
```
Base de datos (MySQL vía Docker):
```
docker compose up -d
```
Tests: **no hay suite automatizada todavía** (Catch2 planeado, no implementado —
ver `.ai/ROADMAP.md`). Verificar cambios manualmente contra entrevistas reales; no
afirmar que algo "pasa los tests" si no existen.

## Arquitectura

Monolito modular en C++20. El frontend (React + TS + Vite) es un cliente puro de la
API REST (`/api/v1/...`) — toda la lógica de negocio vive en el backend.

```
Controller → Service → Repository/Interfaz de infraestructura → MySQL / whisper.cpp / Ollama
```

Toda dependencia externa está detrás de una interfaz (`ITranscriber`, `ILLMClient`,
`IInterviewRepository`, etc., en `backend/api/include/`). Un controller nunca debe
acceder a `DatabaseManager` ni contener reglas de negocio.

Detalle completo y el porqué de cada decisión: `.ai/PROJECT.md`, `.ai/ARCHITECTURE.md`,
`.ai/DECISIONS.md` (ADRs), `hermes-vault-knowledge/` (base Obsidian).

## Reglas de estilo (C++)

- Clases `PascalCase`, funciones `camelCase`, miembros privados `m_...`, interfaces
  `IInterface`, constantes `UPPER_CASE`, namespace `hermes::<módulo>`.
- Un archivo por clase. Composición sobre herencia. Sin variables globales.
- RAII siempre. Nunca `new`/`delete` crudo. `unique_ptr` por defecto; `shared_ptr`
  solo si el ownership es realmente compartido.
- Detalle completo: `.ai/CODING_STANDARD.md`.

## Reglas al generar código

- Nunca lógica de negocio en controllers. Nunca acceso directo a DB desde controllers.
- Siempre crear una interfaz antes de depender de infraestructura nueva.
- Preferir standard library sobre dependencias de terceros; no optimizar prematuramente.
- Al introducir una dependencia nueva, explicar la decisión (y registrarla como ADR —
  ver skill `adr-record`).
- Ante ambigüedad de arquitectura o alcance, preguntar en vez de asumir.
- Fuente completa: `.ai/AI_INSTRUCTIONS.md`.

## Fuera de alcance (no agregar sin aprobación explícita del usuario)

Autenticación, procesamiento cloud (OpenAI/Azure/AWS/GCP), microservicios,
Kubernetes, message brokers, infraestructura distribuida compleja.
(`.ai/PROJECT.md` § Out of Scope)

## Privacidad — no es una feature más, es la razón de ser del proyecto

Ninguna PII debe salir de la máquina local salvo configuración explícita del usuario.
Cualquier llamada de red nueva hacia algo que no sea `localhost` (Ollama, MySQL local)
debe cuestionarse explícitamente antes de escribirse. La anonimización actual **no**
tiene 100% de recall (ver README § Limitaciones conocidas) — no lo des por sentado al
tocar `TranscriptEnhancer` o el pipeline de anonimización.

## Contexto vivo del proyecto

- `.ai/` — decisiones (ADRs), roadmap por sprint, prompts LLM exactos, backlog de research.
- `hermes-vault-knowledge/` — base de conocimiento Obsidian (ADRs extensos, hallazgos de research).
- `docs/` — requisitos de API y decisiones técnicas del backend.
