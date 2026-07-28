import { apiClient } from "./client";
import type {
  CreateInterviewPayload,
  CreateInterviewResponse,
  Interview,
  InterviewDetail,
  ProcessInterviewResponse,
  UploadAudioResponse,
} from "./types";

// GET /interviews -- todavia no existe en el backend (ver docs/API_REQUIREMENTS.md #2).
export function listInterviews() {
  return apiClient.get<Interview[]>("/interviews");
}

// GET /interview/:id -- todavia no existe en el backend (ver docs/API_REQUIREMENTS.md #2).
export function getInterview(id: number) {
  return apiClient.get<InterviewDetail>(`/interview/${id}`);
}

// POST /interview -- existe, pero hoy no devuelve "id" (ver docs/API_REQUIREMENTS.md #1).
export function createInterview(payload: CreateInterviewPayload) {
  return apiClient.post<CreateInterviewResponse>("/interview", payload);
}

// POST /upload -- existe y funciona tal cual.
export function uploadAudio(interviewId: number, file: File) {
  const form = new FormData();
  form.append("file", file);
  return apiClient.postForm<UploadAudioResponse>("/upload", form, {
    interview_id: String(interviewId),
  });
}

// POST /interview/:id/process -- todavia no existe en el backend (ver docs/API_REQUIREMENTS.md #2).
export function processInterview(id: number) {
  return apiClient.post<ProcessInterviewResponse>(`/interview/${id}/process`);
}
