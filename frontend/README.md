# Hermes — Frontend

Frontend mínimo (sin login) para crear entrevistas, asociarles un audio y
enviarlas a procesar. React + TypeScript + Vite, sin librerías de UI.

## Requisitos

- Node 18+
- El backend (`backend/build/backend.exe`) y el contenedor de MySQL
  (`docker compose up -d mysql`) corriendo — ver la raíz del repo.

## Desarrollo

```bash
npm install
npm run dev
```

Abre en `http://localhost:5173`. Las llamadas a `/api/*` se reenvían al
backend en `http://127.0.0.1:18080` (ver `vite.config.ts`) — no hace falta
configurar CORS para desarrollo.

## Build

```bash
npm run build
```

Genera `dist/`. Para servirlo en producción, ver la sección de CORS en
`../docs/API_REQUIREMENTS.md`.

## Qué falta del lado del backend

Varias pantallas dependen de endpoints que todavía no existen en la API de
Crow (listado de entrevistas, detalle, disparar procesamiento). El detalle
completo, con la forma exacta de request/response esperada, está en
[`../docs/API_REQUIREMENTS.md`](../docs/API_REQUIREMENTS.md). Mientras esos
endpoints no existan, la UI lo muestra explícitamente en vez de fallar en
silencio.
