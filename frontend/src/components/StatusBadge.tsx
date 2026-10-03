import type { InterviewStatus } from "../api/types";
import { STATUS_META } from "../statusMeta";

export function StatusBadge({ status, detail }: { status: InterviewStatus; detail?: string }) {
  const meta = STATUS_META[status];
  return (
    <span className={`badge ${meta.className}`}>
      <span className="badge-dot" aria-hidden="true" />
      {meta.label}
      {detail && <span className="badge-detail"> · {detail}</span>}
    </span>
  );
}
