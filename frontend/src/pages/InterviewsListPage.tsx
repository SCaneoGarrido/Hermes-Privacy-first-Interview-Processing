import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { listInterviews } from "../api/interviews";
import type { Interview } from "../api/types";
import { ErrorBanner } from "../components/ErrorBanner";
import { StatusBadge } from "../components/StatusBadge";

export function InterviewsListPage() {
  const [interviews, setInterviews] = useState<Interview[] | null>(null);
  const [error, setError] = useState<unknown>(null);

  useEffect(() => {
    let cancelled = false;
    listInterviews()
      .then((data) => {
        if (!cancelled) setInterviews(data);
      })
      .catch((err) => {
        if (!cancelled) setError(err);
      });
    return () => {
      cancelled = true;
    };
  }, []);

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
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </section>
  );
}
