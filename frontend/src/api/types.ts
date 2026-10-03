// Espejo de docs/API_REQUIREMENTS.md. Mantener sincronizado con el backend.

export type InterviewStatus =
  | "pending_audio"
  | "pending_processing"
  | "processing"
  | "completed"
  | "failed";

export interface Interview {
  id: number;
  date: string;
  type: string;
  subject_type: string;
  status: InterviewStatus;
  created_at?: string;
}

export interface InterviewAudio {
  path: string;
  format: string;
  size: number;
}

export interface InterviewResult {
  transcription_file_path: string;
  summary_file_path: string | null;
}

// Pasos gruesos de current_step mientras status="processing" (ver
// InterviewProcessingJobHandler en el backend). null si no hay un job
// corriendo en este momento.
export type ProcessingStep =
  | "normalizando_audio"
  | "transcribiendo"
  | "aplicando_glosario"
  | "corrigiendo_texto"
  | "anonimizando"
  | "generando_resumen";

export interface InterviewDetail extends Interview {
  audio: InterviewAudio | null;
  result: InterviewResult | null;
  execution_time_seconds: number | null;
  current_step: ProcessingStep | null;
  keywords: string[];
}

export interface UpdateKeywordsResponse {
  id: number;
  keywords: string[];
}

export interface CreateInterviewPayload {
  date: string;
  type: string;
  subject_type: string;
}

export interface CreateInterviewResponse {
  code: string;
  id: number;
}

export interface UploadAudioResponse {
  filename: string;
  path: string;
  code: string;
}

export interface ProcessInterviewResponse {
  id: number;
  status: InterviewStatus;
  include_summary: boolean;
  enhance_transcript: boolean;
}

export interface DeleteInterviewResponse {
  id: number;
  code: string;
}

// GET /interview/:id/transcript -- transcripcion estructurada para leer en
// la app (ver TranscriptDocumentBuilder en el backend).
export interface TranscriptBlock {
  speaker: string | null;
  start: number | null; // segundo del audio; solo en texto plano de whisper
  text: string;
}

export interface TranscriptDocument {
  id: number;
  has_speakers: boolean;
  has_timestamps: boolean;
  notice: string | null; // aviso del pipeline si la anonimizacion pedida fallo
  summary: string | null;
  blocks: TranscriptBlock[];
}
