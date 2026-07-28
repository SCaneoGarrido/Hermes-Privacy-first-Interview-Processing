import type { InterviewStatus } from "../api/types";
import { STATUS_META } from "../statusMeta";

export function StatusBadge({ status }: { status: InterviewStatus }) {
  const meta = STATUS_META[status];
  return <span className={`badge ${meta.className}`}>{meta.label}</span>;
}
