import { ApiError } from "../api/client";

// Mensajes pensados para la persona usuaria segun el codigo de error de la
// API; si no hay uno especifico se muestra el mensaje que mando el backend.
const FRIENDLY_TITLES: Record<string, string> = {
  NETWORK_ERROR: "No se pudo contactar al backend",
  INTERVIEW_BUSY: "La entrevista se está procesando",
  PAYLOAD_TOO_LARGE: "El archivo es demasiado grande",
  NOT_FOUND: "No encontrado",
};

export function ErrorBanner({ error }: { error: unknown }) {
  if (!error) return null;

  const message = error instanceof ApiError ? error.message : "Ocurrió un error inesperado.";
  const code = error instanceof ApiError ? error.code : "UNKNOWN_ERROR";

  return (
    <div className="banner banner-error" role="alert">
      <strong>{FRIENDLY_TITLES[code] ?? "Error"}</strong>
      <p>{message}</p>
      <p className="banner-hint">Código: {code}</p>
    </div>
  );
}
