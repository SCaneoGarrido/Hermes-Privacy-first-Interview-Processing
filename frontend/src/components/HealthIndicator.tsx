import { useEffect, useState } from "react";
import { getHealth } from "../api/health";

type ConnectionState = "checking" | "online" | "offline";

const POLL_INTERVAL_MS = 15000;

export function HealthIndicator() {
  const [state, setState] = useState<ConnectionState>("checking");

  useEffect(() => {
    let cancelled = false;

    async function check() {
      try {
        await getHealth();
        if (!cancelled) setState("online");
      } catch {
        if (!cancelled) setState("offline");
      }
    }

    check();
    const interval = setInterval(check, POLL_INTERVAL_MS);
    return () => {
      cancelled = true;
      clearInterval(interval);
    };
  }, []);

  const label = { checking: "Verificando…", online: "Backend conectado", offline: "Backend no disponible" }[state];

  return (
    <div className={`health-indicator health-${state}`} title={label}>
      <span className="health-dot" />
      {label}
    </div>
  );
}
