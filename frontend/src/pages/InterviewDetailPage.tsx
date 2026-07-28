import { FormEvent, useCallback, useEffect, useState } from "react";
import { useParams } from "react-router-dom";
import { getInterview, processInterview, uploadAudio } from "../api/interviews";
import type { InterviewDetail, UploadAudioResponse } from "../api/types";
import { ApiError } from "../api/client";
import { ErrorBanner } from "../components/ErrorBanner";
import { StatusBadge } from "../components/StatusBadge";

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

  const loadDetail = useCallback(() => {
    setDetailError(null);
    getInterview(interviewId)
      .then(setDetail)
      .catch(setDetailError);
  }, [interviewId]);

  useEffect(() => {
    loadDetail();
  }, [loadDetail]);

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
      const result = await processInterview(interviewId);
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
        <button onClick={handleProcess} disabled={processing}>
          {processing ? "Enviando…" : "Enviar a procesar"}
        </button>

        <ErrorBanner error={processError} />
        {processNotice && (
          <div className="banner banner-success" role="status">
            <p>{processNotice}</p>
          </div>
        )}
      </div>
    </section>
  );
}
