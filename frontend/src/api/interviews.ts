import { apiClient } from "./client";
import type {
  CreateInterviewPayload,
  CreateInterviewResponse,
  DeleteInterviewResponse,
  Interview,
  InterviewDetail,
  ProcessInterviewResponse,
  UploadAudioResponse,
} from "./types";

export function listInterviews() {
  return apiClient.get<Interview[]>("/interviews");
}

export function getInterview(id: number) {
  return apiClient.get<InterviewDetail>(`/interview/${id}`);
}

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

export function processInterview(id: number) {
  return apiClient.post<ProcessInterviewResponse>(`/interview/${id}/process`);
}

export function deleteInterview(id: number) {
  return apiClient.del<DeleteInterviewResponse>(`/interview/${id}`);
}
