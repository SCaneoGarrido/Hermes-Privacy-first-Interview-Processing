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
}

export interface InterviewDetail extends Interview {
  audio: InterviewAudio | null;
  result: InterviewResult | null;
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
}

export interface DeleteInterviewResponse {
  id: number;
  code: string;
}
