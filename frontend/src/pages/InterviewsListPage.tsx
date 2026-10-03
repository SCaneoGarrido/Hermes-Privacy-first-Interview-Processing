import { useCallback, useEffect, useMemo, useState } from "react";
import { Link } from "react-router-dom";
import { deleteInterview, getInterview, listInterviews } from "../api/interviews";
import type { Interview, InterviewStatus, ProcessingStep } from "../api/types";
import { ConfirmDialog } from "../components/ConfirmDialog";
import { ErrorBanner } from "../components/ErrorBanner";
import { HermesMark } from "../components/HermesMark";
import {
  ArchiveIcon,
  BookIcon,
  DownloadIcon,
  EyeIcon,
  HourglassIcon,
  PlayIcon,
  PlusIcon,
  RefreshIcon,
  SearchIcon,
  TrashIcon,
  UploadIcon,
} from "../components/Icons";
import { StatusBadge } from "../components/StatusBadge";
import { dateSortKey, formatInterviewDate } from "../format";
import { STEP_INFO } from "../processingSteps";

type StatusFilter = "all" | InterviewStatus;
type SortOrder = "newest" | "oldest";

const FILTERS: { value: StatusFilter; label: string }[] = [
  { value: "all", label: "Todas" },
  { value: "completed", label: "Completadas" },
  { value: "processing", label: "En proceso" },
  { value: "pending_processing", label: "Listas para procesar" },
  { value: "pending_audio", label: "Falta audio" },
  { value: "failed", label: "Con error" },
];

// Mientras haya algo procesandose, la lista se refresca sola para que el
// estado y el paso de esas filas no queden congelados.
const LIST_POLL_MS = 8000;

export function InterviewsListPage() {
  const [interviews, setInterviews] = useState<Interview[] | null>(null);
  const [error, setError] = useState<unknown>(null);
  const [steps, setSteps] = useState<Record<number, ProcessingStep | null>>({});

  const [query, setQuery] = useState("");
  const [statusFilter, setStatusFilter] = useState<StatusFilter>("all");
  const [sortOrder, setSortOrder] = useState<SortOrder>("newest");

  const [pendingDelete, setPendingDelete] = useState<Interview | null>(null);
  const [deleting, setDeleting] = useState(false);

  const loadInterviews = useCallback(() => {
    return listInterviews()
      .then((data) => {
        setInterviews(data);
        setError(null);
      })
      .catch((err) => setError(err));
  }, []);

  useEffect(() => {
    loadInterviews();
  }, [loadInterviews]);

  // GET /interviews no trae el paso actual: se pide el detalle solo de las
  // (pocas) entrevistas en proceso.
  const processingIds = useMemo(
    () => (interviews ?? []).filter((i) => i.status === "processing").map((i) => i.id),
    [interviews]
  );
  const processingKey = processingIds.join(",");

  useEffect(() => {
    if (processingIds.length === 0) return;
    let cancelled = false;
    Promise.all(
      processingIds.map((id) =>
        getInterview(id)
          .then((detail) => [id, detail.current_step] as const)
          .catch(() => [id, null] as const)
      )
    ).then((entries) => {
      if (!cancelled) setSteps(Object.fromEntries(entries));
    });
    const pollId = setInterval(loadInterviews, LIST_POLL_MS);
    return () => {
      cancelled = true;
      clearInterval(pollId);
    };
    // processingKey resume processingIds: solo re-dispara si cambia el conjunto.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [processingKey, loadInterviews]);

  const counts = useMemo(() => {
    const result: Record<StatusFilter, number> = {
      all: 0,
      pending_audio: 0,
      pending_processing: 0,
      processing: 0,
      completed: 0,
      failed: 0,
    };
    for (const interview of interviews ?? []) {
      result.all++;
      result[interview.status]++;
    }
    return result;
  }, [interviews]);

  const visible = useMemo(() => {
    const needle = query.trim().toLowerCase();
    return (interviews ?? [])
      .filter((i) => statusFilter === "all" || i.status === statusFilter)
      .filter(
        (i) =>
          !needle ||
          i.type.toLowerCase().includes(needle) ||
          i.subject_type.toLowerCase().includes(needle) ||
          String(i.id).includes(needle)
      )
      .sort((a, b) => {
        const cmp = dateSortKey(a.date).localeCompare(dateSortKey(b.date)) || a.id - b.id;
        return sortOrder === "newest" ? -cmp : cmp;
      });
  }, [interviews, query, statusFilter, sortOrder]);

  async function confirmDelete() {
    if (!pendingDelete) return;
    setDeleting(true);
    try {
      await deleteInterview(pendingDelete.id);
      setPendingDelete(null);
      await loadInterviews();
    } catch (err) {
      setPendingDelete(null);
      setError(err);
    } finally {
      setDeleting(false);
    }
  }

  return (
    <section>
      <div className="page-hero">
        <div>
          <p className="eyebrow">
            <span lang="grc">Ἑρμῆς</span> · Repositorio local
          </p>
          <h1 className="display-title">Corpus de entrevistas</h1>
          <p className="lede">
            Tus entrevistas de investigación cualitativa. Los audios y las transcripciones se guardan y procesan en
            este equipo.
          </p>
        </div>
        <Link to="/interviews/new" className="btn btn-primary">
          <PlusIcon /> Nueva entrevista
        </Link>
      </div>

      <div className="meander meander-subtle" aria-hidden="true" />

      <div className="stat-grid">
        <div className="stat-card">
          <div>
            <span className="stat-label">Total catalogadas</span>
            <span className="stat-value">
              {counts.all} <small>entrevistas</small>
            </span>
          </div>
          <ArchiveIcon size={26} />
        </div>
        <div className="stat-card">
          <div>
            <span className="stat-label">En procesamiento</span>
            <span className="stat-value">
              {counts.processing} <small>en curso</small>
            </span>
          </div>
          <HourglassIcon size={26} />
        </div>
        <div className="stat-card">
          <div>
            <span className="stat-label">Listas para descargar</span>
            <span className="stat-value">
              {counts.completed} <small>completadas</small>
            </span>
          </div>
          <DownloadIcon size={26} />
        </div>
      </div>

      <div className="toolbar">
        <div className="toolbar-row">
          <label className="search-field">
            <SearchIcon />
            <span className="visually-hidden">Filtrar entrevistas</span>
            <input
              type="search"
              placeholder="Filtrar por tipo, sujeto o número…"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
            />
          </label>
          <label className="sort-field">
            <span>Ordenar</span>
            <select value={sortOrder} onChange={(e) => setSortOrder(e.target.value as SortOrder)}>
              <option value="newest">Fecha: más recientes primero</option>
              <option value="oldest">Fecha: más antiguas primero</option>
            </select>
          </label>
        </div>
        <div className="chip-row" role="group" aria-label="Filtrar por estado">
          {FILTERS.filter((f) => f.value !== "failed" || counts.failed > 0).map((filter) => (
            <button
              key={filter.value}
              type="button"
              className={`chip${statusFilter === filter.value ? " chip-active" : ""}`}
              aria-pressed={statusFilter === filter.value}
              onClick={() => setStatusFilter(filter.value)}
            >
              {filter.label} <span className="chip-count">({counts[filter.value]})</span>
            </button>
          ))}
        </div>
      </div>

      <ErrorBanner error={error} />

      {!interviews && !error && (
        <div className="interview-list" aria-busy="true">
          {[0, 1, 2].map((n) => (
            <div key={n} className="interview-card skeleton-card">
              <span className="skeleton skeleton-line short" />
              <span className="skeleton skeleton-line" />
              <span className="skeleton skeleton-line medium" />
            </div>
          ))}
        </div>
      )}

      {interviews && interviews.length === 0 && (
        <div className="empty-state">
          <HermesMark size={72} />
          <h2>Todavía no hay entrevistas</h2>
          <p className="muted">
            Creá la primera: cargás sus datos, subís el audio y Hermes lo transcribe en tu equipo.
          </p>
          <Link to="/interviews/new" className="btn btn-primary">
            <PlusIcon /> Crear la primera entrevista
          </Link>
        </div>
      )}

      {interviews && interviews.length > 0 && visible.length === 0 && (
        <p className="muted empty-filter">Ninguna entrevista coincide con el filtro.</p>
      )}

      {visible.length > 0 && (
        <ul className="interview-list">
          {visible.map((interview) => (
            <InterviewCard
              key={interview.id}
              interview={interview}
              step={steps[interview.id] ?? null}
              onDelete={() => setPendingDelete(interview)}
            />
          ))}
        </ul>
      )}

      {pendingDelete && (
        <ConfirmDialog
          title={`¿Eliminar la entrevista #${pendingDelete.id}?`}
          message={`Se borran sus datos, el audio y las transcripciones (${formatInterviewDate(
            pendingDelete.date
          )}). Esta acción no se puede deshacer.`}
          confirmLabel="Eliminar"
          busy={deleting}
          onConfirm={confirmDelete}
          onCancel={() => setPendingDelete(null)}
        />
      )}
    </section>
  );
}

interface InterviewCardProps {
  interview: Interview;
  step: ProcessingStep | null;
  onDelete: () => void;
}

function InterviewCard({ interview, step, onDelete }: InterviewCardProps) {
  const detailUrl = `/interviews/${interview.id}`;
  const badgeDetail = interview.status === "processing" && step ? STEP_INFO[step].label : undefined;

  return (
    <li className={`interview-card status-${interview.status}`}>
      <div className="interview-card-main">
        <p className="interview-card-meta">
          <span className="interview-number">Entrevista #{interview.id}</span>
          <span className="meta-sep" aria-hidden="true">
            ·
          </span>
          <span className="mono">{formatInterviewDate(interview.date)}</span>
        </p>
        <h2 className="interview-card-title">
          <Link to={detailUrl}>{interview.subject_type}</Link>
        </h2>
        <p className="interview-card-sub">{interview.type}</p>
      </div>

      <div className="interview-card-side">
        <StatusBadge status={interview.status} detail={badgeDetail} />
        <div className="interview-card-actions">
          <PrimaryAction interview={interview} detailUrl={detailUrl} />
          <button
            type="button"
            className="icon-button"
            onClick={onDelete}
            aria-label={`Eliminar la entrevista #${interview.id}`}
            title="Eliminar"
          >
            <TrashIcon />
          </button>
        </div>
      </div>
    </li>
  );
}

// Una sola accion principal por fila, segun el estado (ver STATUS_META).
function PrimaryAction({ interview, detailUrl }: { interview: Interview; detailUrl: string }) {
  switch (interview.status) {
    case "processing":
      return (
        <Link to={detailUrl} className="btn btn-ghost">
          <RefreshIcon /> Ver progreso
        </Link>
      );
    case "completed":
      return (
        <>
          <Link to={`/interviews/${interview.id}/transcript`} className="btn btn-secondary">
            <BookIcon /> Leer
          </Link>
          <Link to={detailUrl} className="btn btn-ghost">
            <EyeIcon /> Ver ficha
          </Link>
        </>
      );
    case "pending_processing":
      return (
        <Link to={detailUrl} className="btn btn-ghost">
          <PlayIcon /> Iniciar procesamiento
        </Link>
      );
    case "pending_audio":
      return (
        <Link to={detailUrl} className="btn btn-ghost">
          <UploadIcon /> Subir audio
        </Link>
      );
    case "failed":
      return (
        <Link to={detailUrl} className="btn btn-ghost">
          <EyeIcon /> Ver detalle
        </Link>
      );
  }
}
