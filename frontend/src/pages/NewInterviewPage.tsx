import { FormEvent, useState } from "react";
import { useNavigate } from "react-router-dom";
import { createInterview } from "../api/interviews";
import { ErrorBanner } from "../components/ErrorBanner";

// "2026-07-28T10:00" (datetime-local) -> "2026-07-28 10:00:00" (MySQL DATETIME)
function toMysqlDatetime(localValue: string): string {
  return localValue.replace("T", " ") + ":00";
}

export function NewInterviewPage() {
  const navigate = useNavigate();
  const [date, setDate] = useState("");
  const [type, setType] = useState("");
  const [subjectType, setSubjectType] = useState("");
  const [error, setError] = useState<unknown>(null);
  const [notice, setNotice] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  async function handleSubmit(event: FormEvent) {
    event.preventDefault();
    setError(null);
    setNotice(null);
    setSubmitting(true);

    try {
      const response = await createInterview({
        date: toMysqlDatetime(date),
        type,
        subject_type: subjectType,
      });

      if (typeof response.id === "number") {
        navigate(`/interviews/${response.id}`);
      } else {
        // El backend actual no devuelve el id (ver docs/API_REQUIREMENTS.md #1).
        // No hay forma de saber a que entrevista asociarle audio despues.
        setNotice(
          "Entrevista creada, pero el backend no devolvió su id: no se puede " +
            "asociar audio automáticamente todavía. Ver docs/API_REQUIREMENTS.md."
        );
      }
    } catch (err) {
      setError(err);
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <section className="form-section">
      <h1>Nueva entrevista</h1>

      <ErrorBanner error={error} />
      {notice && (
        <div className="banner banner-warning" role="status">
          <p>{notice}</p>
        </div>
      )}

      <form onSubmit={handleSubmit} className="form">
        <label>
          Fecha y hora
          <input type="datetime-local" required value={date} onChange={(e) => setDate(e.target.value)} />
        </label>

        <label>
          Tipo de entrevista
          <input
            type="text"
            required
            placeholder="p. ej. técnica, investigación cualitativa"
            value={type}
            onChange={(e) => setType(e.target.value)}
          />
        </label>

        <label>
          Tipo de sujeto
          <input
            type="text"
            required
            placeholder="p. ej. candidato, entrevistado"
            value={subjectType}
            onChange={(e) => setSubjectType(e.target.value)}
          />
        </label>

        <button type="submit" disabled={submitting}>
          {submitting ? "Creando…" : "Crear entrevista"}
        </button>
      </form>
    </section>
  );
}
