import { ApiError, apiClient } from "./client";
import type {
  CreateInterviewPayload,
  CreateInterviewResponse,
  DeleteInterviewResponse,
  Interview,
  InterviewDetail,
  ProcessInterviewResponse,
  TranscriptBlock,
  TranscriptDocument,
  UpdateKeywordsResponse,
  UploadAudioResponse,
} from "./types";

// Mismos limites que Config::MAX_KEYWORDS / MAX_KEYWORD_LENGTH en el backend.
export const MAX_KEYWORDS = 100;
export const MAX_KEYWORD_LENGTH = 80;

// Texto del archivo de palabras clave -> lista. Acepta coma, punto y coma o
// salto de linea como separador; descarta vacios y duplicados (sin distinguir
// mayusculas). El backend vuelve a validar.
export function parseKeywords(text: string): string[] {
  const seen = new Set<string>();
  const keywords: string[] = [];
  for (const raw of text.split(/[,;\r\n]+/)) {
    const keyword = raw.trim();
    if (!keyword || seen.has(keyword.toLowerCase())) continue;
    seen.add(keyword.toLowerCase());
    keywords.push(keyword);
  }
  return keywords;
}

// PUT /interview/:id/keywords -- reemplaza el glosario; [] lo borra.
export function updateKeywords(id: number, keywords: string[]) {
  return apiClient.put<UpdateKeywordsResponse>(`/interview/${id}/keywords`, { keywords });
}

export function listInterviews() {
  return apiClient.get<Interview[]>("/interviews");
}

export function getInterview(id: number) {
  return apiClient.get<InterviewDetail>(`/interview/${id}`);
}

export function createInterview(payload: CreateInterviewPayload) {
  return apiClient.post<CreateInterviewResponse>("/interview", payload);
}

// Mismo tope que Config::MAX_UPLOAD_BYTES en el backend (1 GiB). Se chequea
// aca para no transferir el archivo entero solo para recibir un 413.
export const MAX_UPLOAD_BYTES = 1024 * 1024 * 1024;

// POST /upload -- acepta audio o video .mp4 (el backend extrae el audio).
export function uploadAudio(interviewId: number, file: File) {
  if (file.size > MAX_UPLOAD_BYTES) {
    return Promise.reject(
      new ApiError(413, "PAYLOAD_TOO_LARGE", "El archivo supera el tamaño máximo permitido (1 GB)")
    );
  }
  const form = new FormData();
  form.append("file", file);
  return apiClient.postForm<UploadAudioResponse>("/upload", form, {
    interview_id: String(interviewId),
  });
}

export function processInterview(id: number, includeSummary: boolean = false, enhanceTranscript: boolean = false) {
  return apiClient.post<ProcessInterviewResponse>(`/interview/${id}/process`, {
    include_summary: includeSummary,
    enhance_transcript: enhanceTranscript,
  });
}

export function getTranscript(id: number) {
  return apiClient.get<TranscriptDocument>(`/interview/${id}/transcript`);
}

// PUT /interview/:id/transcript -- guarda la version editada (la original
// del pipeline se conserva). Devuelve el documento actualizado.
export function updateTranscript(id: number, blocks: TranscriptBlock[]) {
  return apiClient.put<TranscriptDocument>(`/interview/${id}/transcript`, { blocks });
}

// DELETE /interview/:id/transcript/edits -- descarta las ediciones.
export function restoreTranscript(id: number) {
  return apiClient.del<TranscriptDocument>(`/interview/${id}/transcript/edits`);
}

export function deleteInterview(id: number) {
  return apiClient.del<DeleteInterviewResponse>(`/interview/${id}`);
}

// Descargas: no pasan por apiClient (ese espera el sobre JSON
// {success,data,error}; estas rutas devuelven el archivo crudo con
// Content-Disposition). Se usan como href de <a download>, no con fetch -
// asi el navegador maneja la descarga nativamente.
export function transcriptDownloadUrl(id: number) {
  return `/api/v1/interview/${id}/download/transcript`;
}

export function summaryDownloadUrl(id: number) {
  return `/api/v1/interview/${id}/download/summary`;
}
