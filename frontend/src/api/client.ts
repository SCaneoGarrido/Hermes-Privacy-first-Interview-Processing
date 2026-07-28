// Envoltorio fino sobre fetch que entiende el contrato uniforme del backend:
// { success, data, error } (ver backend/api/rules/contract.md). Cualquier
// success:false o fallo de red se convierte en ApiError, asi las pantallas
// solo necesitan un try/catch en vez de repetir el parseo del envelope.

export class ApiError extends Error {
  readonly status: number;
  readonly code: string;

  constructor(status: number, code: string, message: string) {
    super(message);
    this.name = "ApiError";
    this.status = status;
    this.code = code;
  }
}

interface ApiEnvelope<T> {
  success: boolean;
  data: T | null;
  error: { code: string; message: string } | null;
}

// Prefijo fijo: Vite lo reescribe hacia el backend real en dev (ver vite.config.ts).
const BASE_URL = "/api";

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  let response: Response;
  try {
    response = await fetch(`${BASE_URL}${path}`, init);
  } catch {
    throw new ApiError(0, "NETWORK_ERROR", "No se pudo contactar al backend. ¿Esta corriendo?");
  }

  let body: ApiEnvelope<T> | null = null;
  try {
    body = (await response.json()) as ApiEnvelope<T>;
  } catch {
    // Respuesta no-JSON (proxy caido, servidor devolviendo HTML, etc.)
  }

  if (!body) {
    throw new ApiError(response.status, "INVALID_RESPONSE", "El backend devolvió una respuesta inesperada.");
  }

  if (!body.success) {
    throw new ApiError(
      response.status,
      body.error?.code ?? "UNKNOWN_ERROR",
      body.error?.message ?? "Error desconocido"
    );
  }

  return body.data as T;
}

export const apiClient = {
  get: <T>(path: string) => request<T>(path),

  post: <T>(path: string, payload?: unknown) =>
    request<T>(path, {
      method: "POST",
      headers: payload !== undefined ? { "Content-Type": "application/json" } : undefined,
      body: payload !== undefined ? JSON.stringify(payload) : undefined,
    }),

  // FormData: no fijar Content-Type a mano, el navegador arma el boundary del multipart.
  postForm: <T>(path: string, form: FormData, extraHeaders?: Record<string, string>) =>
    request<T>(path, { method: "POST", body: form, headers: extraHeaders }),

  del: <T>(path: string) => request<T>(path, { method: "DELETE" }),
};
