import { FormEvent, useCallback, useEffect, useRef, useState } from "react";
import { useParams } from "react-router-dom";
import { getInterview, processInterview, summaryDownloadUrl, transcriptDownloadUrl, uploadAudio } from "../api/interviews";
import type { InterviewDetail, ProcessingStep, UploadAudioResponse } from "../api/types";
import { ApiError } from "../api/client";
import { ErrorBanner } from "../components/ErrorBanner";
import { StatusBadge } from "../components/StatusBadge";

// Etiquetas legibles para current_step (ver InterviewProcessingJobHandler
// en el backend). Un procesamiento real puede tardar 10-20+ minutos sin
// esto - sin feedback intermedio, parece trabado aunque este funcionando.
const STEP_LABELS: Record<ProcessingStep, string> = {
  normalizando_audio: "Normalizando audio",
  transcribiendo: "Transcribiendo con IA (whisper.cpp) — suele ser el paso más largo",
  corrigiendo_texto: "Corrigiendo texto con IA",
  anonimizando: "Anonimizando información personal",
  generando_resumen: "Generando resumen",
};

function formatElapsed(seconds: number): string {
  const m = Math.floor(seconds / 60);
  const s = seconds % 60;
  return `${m}:${String(s).padStart(2, "0")}`;
}

export function InterviewDetailPage() {
  const { id } = useParams<{ id: string }>();
  const interviewId = Number(id);

  const [detail, setDetail] = useState<InterviewDetail | null>(null);
  const [detailError, setDetailError] = useState<unknown>(null);

  const [file, setFile] = useState<File | null>(null);
  const [uploadResult, setUploadResult] = useState<UploadAudioResponse | null>(null);
  const [uploadError, setUploadError] = useState<unknown>(null);
  const [uploading, setUploading] = useState(false);

  const [processError, setProcessError] = useState<unknown>(null);
  const [processNotice, setProcessNotice] = useState<string | null>(null);
  const [processing, setProcessing] = useState(false);
  const [includeSummary, setIncludeSummary] = useState(false);

  // Momento en que se detectó status="processing", para mostrar tiempo
  // transcurrido. No usa execution_time_seconds (eso es del último job ya
  // terminado) - esto es "cuánto lleva el que está corriendo ahora".
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

  const detailUnavailable = detailError instanceof ApiError && detailError.code === "NOT_FOUND";

  async function handleUpload(event: FormEvent) {
    event.preventDefault();
    if (!file) return;

    setUploadError(null);
    setUploadResult(null);
    setUploading(true);
    try {
      const result = await uploadAudio(interviewId, file);
      setUploadResult(result);
      loadDetail();
    } catch (err) {
      setUploadError(err);
    } finally {
      setUploading(false);
    }
  }

  async function handleProcess() {
    setProcessError(null);
    setProcessNotice(null);
    setProcessing(true);
    try {
      const result = await processInterview(interviewId, includeSummary);
      setProcessNotice(`Estado actualizado a "${result.status}".`);
      loadDetail();
    } catch (err) {
      setProcessError(err);
    } finally {
      setProcessing(false);
    }
  }

  return (
    <section>
      <div className="page-header">
        <h1>Entrevista #{interviewId}</h1>
        {detail && <StatusBadge status={detail.status} />}
      </div>

      {detail?.status === "processing" && (
        <div className="banner banner-warning" role="status">
          <p>
            {detail.current_step ? STEP_LABELS[detail.current_step] : "Iniciando procesamiento…"} — en curso desde
            hace {formatElapsed(elapsedSeconds)}. Los audios largos pueden tardar 10-20+ minutos, sobre todo en la
            transcripción; esto no significa que quedó trabado.
          </p>
        </div>
      )}

      {detailUnavailable ? (
        <div className="banner banner-warning" role="status">
          <p>
            El backend todavía no expone <code>GET /interview/:id</code>, así que no se puede mostrar el detalle
            completo (ver <code>docs/API_REQUIREMENTS.md</code>). Igual podés subir un audio y enviarlo a procesar
            usando el id de la URL.
          </p>
        </div>
      ) : (
        <ErrorBanner error={detailError} />
      )}

      {detail && (
        <dl className="details-grid">
          <dt>Fecha</dt>
          <dd>{detail.date}</dd>
          <dt>Tipo</dt>
          <dd>{detail.type}</dd>
          <dt>Sujeto</dt>
          <dd>{detail.subject_type}</dd>
        </dl>
      )}

      <div className="card">
        <h2>Audio</h2>

        {detail?.audio ? (
          <p>
            Audio asociado: <code>{detail.audio.path}</code> ({detail.audio.format}, {detail.audio.size} bytes)
          </p>
        ) : (
          <p className="muted">Todavía no hay audio asociado a esta entrevista.</p>
        )}

        <form onSubmit={handleUpload} className="form form-inline">
          <input
            type="file"
            accept="audio/mpeg,audio/wav,audio/ogg,audio/x-m4a,.mp3,.wav,.ogg,.m4a"
            onChange={(e) => setFile(e.target.files?.[0] ?? null)}
          />
          <button type="submit" disabled={!file || uploading}>
            {uploading ? "Subiendo…" : "Subir audio"}
          </button>
        </form>

        <ErrorBanner error={uploadError} />
        {uploadResult && (
          <div className="banner banner-success" role="status">
            <p>
              Audio guardado en <code>{uploadResult.path}</code>.
            </p>
          </div>
        )}
      </div>

      <div className="card">
        <h2>Procesamiento</h2>
        <p className="muted">
          Dispara la transcripción de la entrevista. Requiere que ya haya un audio asociado.
        </p>

        <label>
          <input
            type="checkbox"
            checked={includeSummary}
            onChange={(e) => setIncludeSummary(e.target.checked)}
          />{" "}
          Generar resumen
        </label>
        {includeSummary && (
          <p className="muted">
            El resumen es opcional y puede aumentar el tiempo de procesamiento en ~30% (llamadas adicionales al
            modelo local de IA sobre la entrevista ya corregida).
          </p>
        )}

        <button onClick={handleProcess} disabled={processing}>
          {processing ? "Enviando…" : "Enviar a procesar"}
        </button>

        <ErrorBanner error={processError} />
        {processNotice && (
          <div className="banner banner-success" role="status">
            <p>{processNotice}</p>
          </div>
        )}

        {detail?.execution_time_seconds != null && (
          <p className="muted">Último procesamiento: {detail.execution_time_seconds} segundos.</p>
        )}
      </div>

      {detail?.result && (
        <div className="card">
          <h2>Resultado</h2>
          <p>
            <a href={transcriptDownloadUrl(interviewId)} download>
              Descargar transcripción (.txt)
            </a>
          </p>
          {detail.result.summary_file_path ? (
            <p>
              <a href={summaryDownloadUrl(interviewId)} download>
                Descargar resumen (.txt)
              </a>
            </p>
          ) : (
            <p className="muted">Sin resumen generado (no se pidió al procesar).</p>
          )}
        </div>
      )}
    </section>
  );
}
