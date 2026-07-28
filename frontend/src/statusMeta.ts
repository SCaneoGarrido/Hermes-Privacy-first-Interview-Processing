import type { InterviewStatus } from "./api/types";

export const STATUS_META: Record<InterviewStatus, { label: string; className: string }> = {
  pending_audio: { label: "Falta audio", className: "badge-neutral" },
  pending_processing: { label: "Listo para procesar", className: "badge-info" },
  processing: { label: "Procesando", className: "badge-warning" },
  completed: { label: "Completado", className: "badge-success" },
  failed: { label: "Error", className: "badge-danger" },
};
