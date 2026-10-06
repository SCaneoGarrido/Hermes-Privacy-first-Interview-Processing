import { FormEvent, useCallback, useEffect, useRef, useState } from "react";
import { Link, useParams } from "react-router-dom";
import { getInterview, processInterview, summaryDownloadUrl, transcriptDownloadUrl, uploadAudio } from "../api/interviews";
import type { InterviewDetail, UploadAudioResponse } from "../api/types";
import { ApiError } from "../api/client";
import { ConfirmDialog } from "../components/ConfirmDialog";
import { Dropzone } from "../components/Dropzone";
import { ErrorBanner } from "../components/ErrorBanner";
import {
  AlertIcon,
  ArrowLeftIcon,
  ClockIcon,
  DownloadIcon,
  EyeIcon,
  FileAudioIcon,
  InfoIcon,
  NoFileIcon,
  PlayIcon,
  ShieldIcon,
  TempleIcon,
} from "../components/Icons";
import { KeywordsCard } from "../components/KeywordsCard";
import { StatusBadge } from "../components/StatusBadge";
import { ThresholdStepper } from "../components/ThresholdStepper";
import { fileNameFromPath, formatBytes, formatClock, formatDuration, formatInterviewDate } from "../format";
import { STEP_INFO, buildStepViews, readProcessOptions, rememberProcessOptions } from "../processingSteps";

const LEDE: Record<InterviewDetail["status"], string> = {
  pending_audio: "Falta subir el audio de la entrevista para poder transcribirla.",
  pending_processing: "El audio está cargado. Elegí las opciones y enviala a procesar.",
  processing: "Hermes está procesando la entrevista en este equipo.",
  completed: "La transcripción está lista para descargar y revisar.",
  failed: "El último procesamiento no pudo completarse.",
};

export function InterviewDetailPage() {
  const { id } = useParams<{ id: string }>();
  const interviewId = Number(id);

  const [detail, setDetail] = useState<InterviewDetail | null>(null);
  const [detailError, setDetailError] = useState<unknown>(null);

  const [file, setFile] = useState<File | null>(null);
  const [uploadResult, setUploadResult] = useState<UploadAudioResponse | null>(null);
  const [uploadError, setUploadError] = useState<unknown>(null);
  const [uploading, setUploading] = useState(false);
  const [confirmReplace, setConfirmReplace] = useState(false);

  const [processError, setProcessError] = useState<unknown>(null);
  const [processing, setProcessing] = useState(false);
  const [confirmReprocess, setConfirmReprocess] = useState(false);
  const [includeSummary, setIncludeSummary] = useState(false);
  const [enhanceTranscript, setEnhanceTranscript] = useState(false);

  // Momento en que esta pagina detecto status="processing". La API no dice
  // cuando arranco el job, asi que el cronometro mide desde que se esta
  // observando y lo aclara en pantalla. No usa execution_time_seconds (eso
  // es del ultimo job ya terminado).
  const processingSinceRef = useRef<number | null>(null);
  const [elapsedSeconds, setElapsedSeconds] = useState(0);

  const loadDetail = useCallback(() => {
    setDetailError(null);
    getInterview(interviewId)
      .then(setDetail)
      .catch(setDetailError);
  }, [interviewId]);

  useEffect(() => {
    loadDetail();
  }, [loadDetail]);

  // Polling mientras hay un job corriendo: sin esto, el usuario ve
  // "processing" estático y no tiene forma de saber si avanzó sin
  // recargar la página a mano.
  useEffect(() => {
    if (detail?.status !== "processing") {
      processingSinceRef.current = null;
      setElapsedSeconds(0);
      return;
    }

    if (processingSinceRef.current === null) {
      processingSinceRef.current = Date.now();
    }

    const pollId = setInterval(loadDetail, 4000);
    const tickId = setInterval(() => {
      setElapsedSeconds(Math.floor((Date.now() - (processingSinceRef.current ?? Date.now())) / 1000));
    }, 1000);

    return () => {
      clearInterval(pollId);
      clearInterval(tickId);
    };
  }, [detail?.status, loadDetail]);

  const notFound = detailError instanceof ApiError && detailError.code === "NOT_FOUND";

  function handleUploadSubmit(event: FormEvent) {
    event.preventDefault();
    if (!file) return;
    // Reemplazar descarta el audio y el resultado anteriores en el backend:
    // se pide confirmacion explicita.
    if (detail?.audio) {
      setConfirmReplace(true);
      return;
    }
    uploadFile();
  }

  async function uploadFile() {
    if (!file) return;

    setUploadError(null);
    setUploadResult(null);
    setUploading(true);
    try {
      const result = await uploadAudio(interviewId, file);
      setUploadResult(result);
      setFile(null);
      loadDetail();
    } catch (err) {
      setUploadError(err);
    } finally {
      setUploading(false);
      setConfirmReplace(false);
    }
  }

  // Reprocesar reemplaza la transcripcion: si tiene ediciones manuales, se
  // pide confirmacion antes de descartarlas.
  function requestProcess() {
    if (detail?.transcript_edited) {
      setConfirmReprocess(true);
      return;
    }
    handleProcess();
  }

  async function handleProcess() {
    setConfirmReprocess(false);
    setProcessError(null);
    setProcessing(true);
    try {
      await processInterview(interviewId, includeSummary, enhanceTranscript);
      rememberProcessOptions(interviewId, { includeSummary, enhanceTranscript });
      loadDetail();
    } catch (err) {
      setProcessError(err);
    } finally {
      setProcessing(false);
    }
  }

  const status = detail?.status;
  const isProcessing = status === "processing";

  return (
    <section>
      <div className="detail-topbar">
        <nav className="breadcrumb" aria-label="Ruta">
          <TempleIcon size={16} />
          <Link to="/">Entrevistas</Link>
          <span aria-hidden="true">/</span>
          <span aria-current="page">Entrevista #{interviewId}</span>
        </nav>
        {detail && (
          <StatusBadge
            status={detail.status}
            detail={isProcessing && detail.current_step ? STEP_INFO[detail.current_step].label : undefined}
          />
        )}
      </div>

      <h1 className="display-title">Entrevista #{interviewId}</h1>
      {status && <p className="lede">{LEDE[status]}</p>}

      {notFound ? (
        <div className="banner banner-error" role="alert">
          <strong>No existe la entrevista #{interviewId}</strong>
          <p>
            Puede que se haya eliminado. <Link to="/">Volver al corpus de entrevistas</Link>.
          </p>
        </div>
      ) : (
        <ErrorBanner error={detailError} />
      )}

      {detail && (
        <div className="card record-card">
          <div className="card-heading">
            <h2 className="section-title">Ficha de la entrevista</h2>
          </div>
          <dl className="record-grid">
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
            <div>
              <dt>Audio</dt>
              {detail.audio ? (
                <dd>
                  <span className="record-file" title={detail.audio.path}>
                    {fileNameFromPath(detail.audio.path)}
                  </span>
                  <span className="record-meta mono">
                    {detail.audio.format} · {formatBytes(detail.audio.size)}
                  </span>
                </dd>
              ) : (
                <dd className="muted record-empty">
                  <NoFileIcon size={16} /> Sin archivo cargado
                </dd>
              )}
            </div>
          </dl>
        </div>
      )}

      {detail && isProcessing && <ProcessingView detail={detail} elapsedSeconds={elapsedSeconds} />}

      {detail && !isProcessing && (
        <>
          {status === "failed" && (
            <div className="card failure-card" role="alert">
              <div className="card-heading">
                <AlertIcon size={20} />
                <h2 className="section-title">No se pudo completar el procesamiento</h2>
              </div>
              <p>
                El motivo queda registrado en el log del backend (<code>API.log</code>). Causas habituales: el archivo
                de audio está dañado, falta el modelo de whisper.cpp, o el backend se reinició a mitad del proceso.
                Podés reintentarlo más abajo.
              </p>
            </div>
          )}

          {detail.result && (
            <ResultCard
              interviewId={interviewId}
              hasSummary={!!detail.result.summary_file_path}
              executionSeconds={detail.execution_time_seconds}
            />
          )}

          <div className="card">
            <div className="card-heading">
              <FileAudioIcon size={20} />
              <h2 className="section-title">{detail.audio ? "Reemplazar audio" : "Subir audio"}</h2>
            </div>
            <form onSubmit={handleUploadSubmit} className="stack">
              <Dropzone file={file} onFileChange={setFile} compact={!!detail.audio} />
              {detail.audio && (
                <p className="muted small">
                  Reemplazar el audio borra el archivo actual{detail.result ? ", la transcripción y el resumen" : ""}.
                  Vas a tener que volver a procesar la entrevista.
                </p>
              )}
              {file && (
                <button type="submit" className="btn btn-primary" disabled={uploading}>
                  {uploading ? "Subiendo…" : detail.audio ? "Reemplazar audio" : "Subir archivo"}
                </button>
              )}
            </form>
            <ErrorBanner error={uploadError} />
            {uploadResult && (
              <div className="banner banner-success" role="status">
                <p>Audio guardado: {uploadResult.filename}.</p>
              </div>
            )}
          </div>

          <KeywordsCard interviewId={interviewId} saved={detail.keywords ?? []} onSaved={loadDetail} />

          <div className="card">
            <div className="card-heading">
              <PlayIcon size={20} />
              <h2 className="section-title">Procesamiento</h2>
            </div>

            {!detail.audio && <p className="muted">Primero subí el audio de la entrevista.</p>}

            <div className="option-list">
              <label className="option">
                <input
                  type="checkbox"
                  className="switch"
                  checked={enhanceTranscript}
                  onChange={(e) => setEnhanceTranscript(e.target.checked)}
                />
                <span>
                  <span className="option-title">
                    Corregir y anonimizar con IA <span className="tag">Experimental</span>
                  </span>
                  <span className="option-hint">
                    Corrige ortografía y puntuación turno por turno y reemplaza nombres por marcadores (los hablantes
                    no cambian). Todavía puede reemplazar términos comunes (ej. "paciente") y omitir nombres. Sin esta
                    opción la transcripción sale tal cual la produce whisper, <strong>sin anonimizar</strong>.
                  </span>
                </span>
              </label>
              <label className="option">
                <input
                  type="checkbox"
                  className="switch"
                  checked={includeSummary}
                  onChange={(e) => setIncludeSummary(e.target.checked)}
                />
                <span>
                  <span className="option-title">Generar resumen</span>
                  <span className="option-hint">
                    Se genera con el modelo local de IA y se descarga como un documento aparte. Aumenta el tiempo de
                    procesamiento.
                  </span>
                </span>
              </label>
            </div>

            <p className="muted expectation">
              <ClockIcon size={16} /> Todo corre en este equipo: una entrevista de una hora puede tardar unos 25
              minutos con GPU, y bastante más solo con CPU.
            </p>

            <button
              type="button"
              className="btn btn-primary"
              onClick={requestProcess}
              disabled={processing || !detail.audio}
            >
              <PlayIcon />
              {processing
                ? "Enviando…"
                : status === "failed"
                  ? "Reintentar procesamiento"
                  : status === "completed"
                    ? "Volver a procesar"
                    : "Enviar a procesar"}
            </button>
            <ErrorBanner error={processError} />
          </div>
        </>
      )}

      {confirmReplace && detail?.audio && file && (
        <ConfirmDialog
          title="¿Reemplazar el audio?"
          message={`Se borra el audio actual (${fileNameFromPath(detail.audio.path)})${
            detail.result ? " junto con la transcripción y el resumen generados" : ""
          }. Esta acción no se puede deshacer.`}
          confirmLabel="Reemplazar"
          busy={uploading}
          busyLabel="Subiendo…"
          onConfirm={uploadFile}
          onCancel={() => setConfirmReplace(false)}
        />
      )}

      {confirmReprocess && (
        <ConfirmDialog
          title="¿Volver a procesar?"
          message="La transcripción tiene ediciones manuales. Al volver a procesar se genera una transcripción nueva y esas ediciones se descartan."
          confirmLabel="Procesar y descartar ediciones"
          busy={processing}
          busyLabel="Enviando…"
          onConfirm={handleProcess}
          onCancel={() => setConfirmReprocess(false)}
        />
      )}

      <div className="detail-footer">
        <Link to="/" className="back-link">
          <ArrowLeftIcon /> Regresar al corpus de entrevistas
        </Link>
      </div>
    </section>
  );
}

function ProcessingView({ detail, elapsedSeconds }: { detail: InterviewDetail; elapsedSeconds: number }) {
  const steps = buildStepViews(detail.current_step, detail.keywords?.length ?? 0, readProcessOptions(detail.id));

  return (
    <>
      <div className="section-header">
        <h2 className="section-title">
          <span className="section-dot" aria-hidden="true" /> Secuencia de umbrales
        </h2>
        <span className="muted small">
          {detail.current_step ? `En curso: ${STEP_INFO[detail.current_step].label}` : "Iniciando procesamiento…"}
        </span>
      </div>
      <ThresholdStepper steps={steps} />

      <div className="processing-grid">
        <div className="card timer-card">
          <div className="card-heading">
            <ClockIcon size={20} />
            <h2 className="section-title">Tiempo observado</h2>
          </div>
          <div className="timer-display" role="timer" aria-live="off">
            <span className="timer-value">{formatClock(elapsedSeconds)}</span>
            <span className="timer-caption">Contado desde que abriste esta página</span>
          </div>
          <p className="timer-footer">
            <span className="live-dot" aria-hidden="true" /> Actualización automática cada 4 s
          </p>
        </div>

        <div className="processing-side">
          <div className="card">
            <div className="card-heading">
              <InfoIcon size={20} />
              <h2 className="section-title">Mientras tanto</h2>
            </div>
            <p className="guide-text">
              Una entrevista de una hora suele tardar <strong>25 minutos o más</strong>, sobre todo durante la
              transcripción y la identificación de hablantes. Que un paso demore no significa que el proceso esté trabado.
            </p>
            <div className="inset-note">
              <strong>Podés cerrar esta pestaña</strong>
              <p>
                El procesamiento sigue en el backend de este equipo. Cuando vuelvas, vas a ver en qué paso está.
              </p>
            </div>
          </div>

          <div className="card privacy-note">
            <ShieldIcon size={20} />
            <p>
              <strong>Procesamiento local.</strong> El audio y el texto se procesan en este equipo con whisper.cpp y
              Ollama; no se envían a servicios externos.
            </p>
          </div>
        </div>
      </div>
    </>
  );
}

interface ResultCardProps {
  interviewId: number;
  hasSummary: boolean;
  executionSeconds: number | null;
}

function ResultCard({ interviewId, hasSummary, executionSeconds }: ResultCardProps) {
  return (
    <div className="card result-card">
      <div className="card-heading">
        <DownloadIcon size={20} />
        <h2 className="section-title">Resultado</h2>
        {executionSeconds != null && (
          <span className="muted small heading-aside">Procesado en {formatDuration(executionSeconds)}</span>
        )}
      </div>

      <div className="result-actions">
        <Link to={`/interviews/${interviewId}/transcript`} className="btn btn-primary">
          <EyeIcon /> Leer transcripción
        </Link>
        <a href={transcriptDownloadUrl(interviewId)} download className="btn btn-secondary">
          <DownloadIcon /> Descargar .txt
        </a>
        {hasSummary ? (
          <a href={summaryDownloadUrl(interviewId)} download className="btn btn-secondary">
            <DownloadIcon /> Descargar resumen (.txt)
          </a>
        ) : (
          <span className="muted small">Sin resumen (no se pidió o no se pudo generar).</span>
        )}
      </div>

      <div className="banner banner-warning review-warning" role="note">
        <AlertIcon size={18} />
        <div>
          <strong>Revisá el texto antes de compartirlo</strong>
          <p>
            Si no activaste "Corregir y anonimizar", la transcripción no está anonimizada. Si lo activaste, la
            anonimización automática puede omitir nombres; y si falló, la primera línea del archivo lo indica.
          </p>
        </div>
      </div>
    </div>
  );
}
