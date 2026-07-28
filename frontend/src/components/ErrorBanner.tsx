import { ApiError } from "../api/client";

export function ErrorBanner({ error }: { error: unknown }) {
  if (!error) return null;

  const message = error instanceof ApiError ? error.message : "Ocurrió un error inesperado.";
  const code = error instanceof ApiError ? error.code : "UNKNOWN_ERROR";
  const isMissingEndpoint = error instanceof ApiError && error.code === "NOT_FOUND";

  return (
    <div className="banner banner-error" role="alert">
      <strong>{isMissingEndpoint ? "Funcionalidad no disponible todavía" : "Error"}</strong>
      <p>{message}</p>
      {isMissingEndpoint && (
        <p className="banner-hint">
          Este endpoint todavía no está implementado en el backend. Ver <code>docs/API_REQUIREMENTS.md</code>.
        </p>
      )}
      {!isMissingEndpoint && <p className="banner-hint">Código: {code}</p>}
    </div>
  );
}
