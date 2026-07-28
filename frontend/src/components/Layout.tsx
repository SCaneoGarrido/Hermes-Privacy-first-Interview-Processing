import type { ReactNode } from "react";
import { Link } from "react-router-dom";
import { HealthIndicator } from "./HealthIndicator";

export function Layout({ children }: { children: ReactNode }) {
  return (
    <div className="app-shell">
      <header className="app-header">
        <Link to="/" className="app-title">
          Hermes
        </Link>
        <nav className="app-nav">
          <Link to="/">Entrevistas</Link>
          <Link to="/interviews/new" className="button-link">
            + Nueva entrevista
          </Link>
        </nav>
        <HealthIndicator />
      </header>
      <main className="app-main">{children}</main>
    </div>
  );
}
