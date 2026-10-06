import { Fragment, ReactNode, useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Link, useNavigate, useParams } from "react-router-dom";
import { ApiError } from "../api/client";
import {
  getInterview,
  getTranscript,
  restoreTranscript,
  transcriptDownloadUrl,
  updateTranscript,
} from "../api/interviews";
import type { InterviewDetail, SpeakerKey, TranscriptBlock, TranscriptDocument } from "../api/types";
import { ConfirmDialog } from "../components/ConfirmDialog";
import { ErrorBanner } from "../components/ErrorBanner";
import { HermesMark } from "../components/HermesMark";
import { AlertIcon, ArrowLeftIcon, DownloadIcon, SearchIcon } from "../components/Icons";
import { TranscriptEditor } from "../components/TranscriptEditor";
import { formatDuration, formatInterviewDate } from "../format";

// Marcadores que deja la anonimizacion ("[PERSONA_1]", "[LUGAR_2]"...): se
// resaltan para que la revision humana los encuentre facil.
const MARKER_PATTERN = /(\[[A-ZÁÉÍÓÚÑ]+(?:_[A-ZÁÉÍÓÚÑ0-9]+)*\])/g;
const MARKER_TEST = /^\[[A-ZÁÉÍÓÚÑ]+(?:_[A-ZÁÉÍÓÚÑ0-9]+)*\]$/;

// 754.3 -> "00:12:34"
function formatTimestamp(seconds: number): string {
  const total = Math.floor(seconds);
  const h = String(Math.floor(total / 3600)).padStart(2, "0");
  const m = String(Math.floor((total % 3600) / 60)).padStart(2, "0");
  const s = String(total % 60).padStart(2, "0");
  return `${h}:${m}:${s}`;
}

function escapeRegExp(value: string): string {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

// Texto -> nodos con marcadores de anonimizacion y coincidencias de busqueda
// resaltados.
function renderText(text: string, query: RegExp | null): ReactNode[] {
  return text.split(MARKER_PATTERN).map((part, i) => {
    if (MARKER_TEST.test(part)) {
      return (
        <span key={i} className="pii-marker">
          {part}
        </span>
      );
    }
    if (!query) return <Fragment key={i}>{part}</Fragment>;
    return (
      <Fragment key={i}>
        {part.split(query).map((piece, j) => (j % 2 === 1 ? <mark key={j}>{piece}</mark> : piece))}
      </Fragment>
    );
  });
}

function paragraphs(text: string): string[] {
  return text
    .split(/\n\s*\n/)
    .map((p) => p.trim())
    .filter(Boolean);
}

// De donde salen los turnos, para que quien lee sepa cuanto confiar en ellos.
function speakerDisclaimer(transcript: TranscriptDocument): string {
  if (!transcript.has_speakers) {
    return "Transcripción automática tal como la produjo whisper, sin etiquetas de hablante. ";
  }
  switch (transcript.speaker_source) {
    case "diarization":
      return "Los hablantes se identificaron automáticamente por la voz y pueden tener errores, sobre todo en intervenciones muy cortas o superpuestas; se corrigen con «Editar». ";
    case "manual":
      return "Los turnos y hablantes fueron revisados y editados a mano. ";
    default:
      return "Los turnos los asignó un modelo de IA a partir del texto (no del audio) y pueden tener errores. ";
  }
}

export function TranscriptReaderPage() {
  const { id } = useParams<{ id: string }>();
  const interviewId = Number(id);

  const [transcript, setTranscript] = useState<TranscriptDocument | null>(null);
  const [detail, setDetail] = useState<InterviewDetail | null>(null);
  const [error, setError] = useState<unknown>(null);
  const [search, setSearch] = useState("");
  const scrollRef = useRef<HTMLDivElement>(null);
  const navigate = useNavigate();

  const [editing, setEditing] = useState(false);
  const [dirty, setDirty] = useState(false);
  const [saving, setSaving] = useState(false);
  const [editError, setEditError] = useState<unknown>(null);
  const [confirmLeave, setConfirmLeave] = useState(false);

  useEffect(() => {
    getTranscript(interviewId).then(setTranscript).catch(setError);
    // La ficha es complementaria: si falla, la transcripcion se lee igual.
    getInterview(interviewId).then(setDetail).catch(() => undefined);
  }, [interviewId]);

  // Cambios sin guardar: el navegador pregunta antes de cerrar/recargar.
  useEffect(() => {
    if (!editing || !dirty) return;
    const onBeforeUnload = (event: BeforeUnloadEvent) => {
      event.preventDefault();
      event.returnValue = "";
    };
    window.addEventListener("beforeunload", onBeforeUnload);
    return () => window.removeEventListener("beforeunload", onBeforeUnload);
  }, [editing, dirty]);

  const onDirtyChange = useCallback((value: boolean) => setDirty(value), []);

  const startEditing = () => {
    setSearch("");
    setEditError(null);
    setEditing(true);
  };

  const stopEditing = () => {
    setEditing(false);
    setDirty(false);
    setEditError(null);
  };

  const save = async (blocks: TranscriptBlock[]) => {
    setSaving(true);
    setEditError(null);
    try {
      setTranscript(await updateTranscript(interviewId, blocks));
      stopEditing();
    } catch (e) {
      setEditError(e);
    } finally {
      setSaving(false);
    }
  };

  const restore = async () => {
    setSaving(true);
    setEditError(null);
    try {
      setTranscript(await restoreTranscript(interviewId));
      stopEditing();
    } catch (e) {
      setEditError(e);
    } finally {
      setSaving(false);
    }
  };

  const labelFor = (key: SpeakerKey | null): string =>
    transcript?.speakers.find((s) => s.key === key)?.label ?? "";

  const query = useMemo(() => {
    const trimmed = search.trim();
    return trimmed.length >= 2 ? new RegExp(`(${escapeRegExp(trimmed)})`, "gi") : null;
  }, [search]);

  const matchCount = useMemo(() => {
    if (!query || !transcript) return 0;
    const all = [transcript.summary ?? "", ...transcript.blocks.map((b) => b.text)].join("\n");
    return all.match(query)?.length ?? 0;
  }, [query, transcript]);

  // Al buscar, el panel con scroll salta a la primera coincidencia.
  useEffect(() => {
    if (!query) return;
    const container = scrollRef.current;
    const first = container?.querySelector("mark");
    if (container && first instanceof HTMLElement) {
      // offsetTop ya es relativo al panel (position: relative en .transcript-scroll).
      container.scrollTo({ top: Math.max(0, first.offsetTop - 24), behavior: "smooth" });
    }
  }, [query]);

  const markerCount = useMemo(() => {
    if (!transcript) return 0;
    return transcript.blocks.reduce((n, b) => n + (b.text.match(MARKER_PATTERN)?.length ?? 0), 0);
  }, [transcript]);

  const unavailable =
    error instanceof ApiError &&
    (error.code === "TRANSCRIPTION_NOT_READY" || error.code === "TRANSCRIPTION_FILE_MISSING");

  return (
    <section className="reader">
      <div className="reader-toolbar">
        <Link
          to={`/interviews/${interviewId}`}
          className="back-link"
          onClick={(e) => {
            if (editing && dirty) {
              e.preventDefault();
              setConfirmLeave(true);
            }
          }}
        >
          <ArrowLeftIcon /> Volver a la entrevista
        </Link>
        {transcript && !editing && (
          <div className="reader-actions">
            <label className="search-field reader-search">
              <SearchIcon />
              <span className="visually-hidden">Buscar en la transcripción</span>
              <input
                type="search"
                placeholder="Buscar en el texto…"
                value={search}
                onChange={(e) => setSearch(e.target.value)}
              />
              {query && (
                <span className="search-count" aria-live="polite">
                  {matchCount} {matchCount === 1 ? "coincidencia" : "coincidencias"}
                </span>
              )}
            </label>
            {transcript.blocks.length > 0 && (
              <button type="button" className="btn btn-secondary" onClick={startEditing}>
                Editar
              </button>
            )}
            <button type="button" className="btn btn-primary" onClick={() => window.print()}>
              Imprimir / Guardar PDF
            </button>
            <a href={transcriptDownloadUrl(interviewId)} download className="btn btn-secondary">
              <DownloadIcon /> .txt
            </a>
          </div>
        )}
      </div>

      {unavailable ? (
        <div className="banner banner-warning" role="status">
          <strong>La transcripción no está disponible</strong>
          <p>{(error as ApiError).message}</p>
          <p>
            <Link to={`/interviews/${interviewId}`}>Ir a la entrevista</Link>
          </p>
        </div>
      ) : (
        <ErrorBanner error={error} />
      )}

      {!transcript && !error && (
        <div className="transcript-doc" aria-busy="true">
          <span className="skeleton skeleton-line short" />
          <span className="skeleton skeleton-line" />
          <span className="skeleton skeleton-line medium" />
        </div>
      )}

      {transcript && (
        <article className="transcript-doc">
          <header className="doc-cover">
            <div className="doc-brand">
              <HermesMark size={34} />
              <span>
                Hermes · <span lang="grc">Ἑρμῆς</span>
              </span>
            </div>
            <p className="eyebrow">Transcripción de entrevista</p>
            <h1 className="display-title">Entrevista #{interviewId}</h1>
            {detail && (
              <dl className="doc-meta">
                <div>
                  <dt>Fecha y hora</dt>
                  <dd>{formatInterviewDate(detail.date)}</dd>
                </div>
                <div>
                  <dt>Tipo de entrevista</dt>
                  <dd>{detail.type}</dd>
                </div>
                <div>
                  <dt>Sujeto</dt>
                  <dd>{detail.subject_type}</dd>
                </div>
                {detail.execution_time_seconds != null && (
                  <div>
                    <dt>Procesamiento</dt>
                    <dd>{formatDuration(detail.execution_time_seconds)}, en este equipo</dd>
                  </div>
                )}
              </dl>
            )}

            {transcript.notice && (
              <div className="doc-alert doc-alert-danger" role="alert">
                <AlertIcon size={18} />
                <p>{transcript.notice.replace(/^\[AVISO HERMES\]\s*/, "")}</p>
              </div>
            )}
            {transcript.edited && transcript.summary && (
              <div className="doc-alert" role="note">
                <AlertIcon size={18} />
                <p>El resumen se generó antes de las ediciones manuales de la transcripción.</p>
              </div>
            )}
            <div className="doc-alert">
              <AlertIcon size={18} />
              <p>
                <strong>Documento para revisión.</strong>{" "}
                {transcript.edited && <span className="doc-badge">Editado manualmente</span>}{" "}
                {speakerDisclaimer(transcript)}
                {markerCount > 0
                  ? `Se reemplazaron ${markerCount} datos personales por marcadores, pero la anonimización automática puede omitir nombres.`
                  : "El texto no contiene marcadores de anonimización: puede incluir nombres y otros datos personales."}{" "}
                Revisalo antes de compartirlo.
              </p>
            </div>
          </header>

          {transcript.summary && (
            <section className="doc-section">
              <h2 className="doc-heading">Resumen</h2>
              {paragraphs(transcript.summary).map((p, i) => (
                <p key={i} className="doc-paragraph">
                  {renderText(p, query)}
                </p>
              ))}
            </section>
          )}

          <section className="doc-section">
            <h2 className="doc-heading">Transcripción</h2>
            {editing ? (
              <>
                <ErrorBanner error={editError} />
                <TranscriptEditor
                  transcript={transcript}
                  saving={saving}
                  onSave={save}
                  onCancel={stopEditing}
                  onRestore={restore}
                  onDirtyChange={onDirtyChange}
                />
              </>
            ) : (
              <>
                {transcript.blocks.length === 0 && <p className="muted">La transcripción está vacía.</p>}
                <div
                  ref={scrollRef}
                  className={`transcript-scroll ${transcript.has_speakers ? "turns" : "passages"}`}
                  tabIndex={0}
                  aria-label="Texto de la transcripción"
                >
                  {transcript.blocks.map((block, i) => (
                    <div key={i} className={`turn${block.speaker ? ` turn-${block.speaker}` : ""}`}>
                      <span className="turn-label">
                        {block.speaker ? labelFor(block.speaker) : block.start != null ? formatTimestamp(block.start) : ""}
                        {block.speaker && block.start != null && (
                          <span className="turn-time">{formatTimestamp(block.start)}</span>
                        )}
                      </span>
                      <p className="turn-text">{renderText(block.text, query)}</p>
                    </div>
                  ))}
                </div>
              </>
            )}
          </section>

          <footer className="doc-footer">
            Generado localmente con Hermes v0.1.0 · Entrevista #{interviewId} · Revisar antes de compartir
          </footer>
        </article>
      )}

      {confirmLeave && (
        <ConfirmDialog
          title="Salir sin guardar"
          message="Hay cambios en la transcripción que no se guardaron y se van a perder."
          confirmLabel="Salir sin guardar"
          onConfirm={() => navigate(`/interviews/${interviewId}`)}
          onCancel={() => setConfirmLeave(false)}
        />
      )}
    </section>
  );
}
