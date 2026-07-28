import { useCallback, useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { deleteInterview, listInterviews } from "../api/interviews";
import type { Interview } from "../api/types";
import { ErrorBanner } from "../components/ErrorBanner";
import { StatusBadge } from "../components/StatusBadge";

export function InterviewsListPage() {
  const [interviews, setInterviews] = useState<Interview[] | null>(null);
  const [error, setError] = useState<unknown>(null);
  const [deletingId, setDeletingId] = useState<number | null>(null);

  const loadInterviews = useCallback(() => {
    return listInterviews()
      .then((data) => setInterviews(data))
      .catch((err) => setError(err));
  }, []);

  useEffect(() => {
    loadInterviews();
  }, [loadInterviews]);

  async function handleDelete(interview: Interview) {
    const confirmed = window.confirm(
      `¿Eliminar la entrevista #${interview.id} (${interview.date})? Esta acción no se puede deshacer.`
    );
    if (!confirmed) return;

    setError(null);
    setDeletingId(interview.id);
    try {
      await deleteInterview(interview.id);
      await loadInterviews();
    } catch (err) {
      setError(err);
    } finally {
      setDeletingId(null);
    }
  }

  return (
    <section>
      <div className="page-header">
        <h1>Entrevistas</h1>
        <Link to="/interviews/new" className="button-link">
          + Nueva entrevista
        </Link>
      </div>

      <ErrorBanner error={error} />

      {!interviews && !error && <p className="muted">Cargando…</p>}

      {interviews && interviews.length === 0 && (
        <p className="muted">Todavía no hay entrevistas. Creá la primera.</p>
      )}

      {interviews && interviews.length > 0 && (
        <table className="table">
          <thead>
            <tr>
              <th>Fecha</th>
              <th>Tipo</th>
              <th>Sujeto</th>
              <th>Estado</th>
              <th />
            </tr>
          </thead>
          <tbody>
            {interviews.map((interview) => (
              <tr key={interview.id}>
                <td>{interview.date}</td>
                <td>{interview.type}</td>
                <td>{interview.subject_type}</td>
                <td>
                  <StatusBadge status={interview.status} />
                </td>
                <td>
                  <Link to={`/interviews/${interview.id}`}>Ver detalle</Link>
                  {" · "}
                  <button
                    type="button"
                    className="button-link-danger"
                    onClick={() => handleDelete(interview)}
                    disabled={deletingId === interview.id}
                  >
                    {deletingId === interview.id ? "Eliminando…" : "Eliminar"}
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </section>
  );
}
