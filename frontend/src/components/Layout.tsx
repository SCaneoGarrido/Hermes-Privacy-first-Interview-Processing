import type { ReactNode } from "react";
import { Link, NavLink } from "react-router-dom";
import { HealthIndicator } from "./HealthIndicator";
import { HermesMark } from "./HermesMark";

export function Layout({ children }: { children: ReactNode }) {
  return (
    <div className="app-shell">
      <header className="app-header">
        <div className="app-header-inner">
          <Link to="/" className="brand" aria-label="Hermes, ir a entrevistas">
            <HermesMark size={40} />
            <span className="brand-text">
              <span className="brand-name">Hermes</span>
              <span className="brand-sub">
                <span lang="grc">Ἑρμῆς</span> · Procesamiento local
              </span>
            </span>
          </Link>

          <nav className="app-nav" aria-label="Principal">
            <NavLink to="/" end>
              Entrevistas
            </NavLink>
            <NavLink to="/interviews/new">Nueva entrevista</NavLink>
          </nav>

          <div className="app-header-status">
            <HealthIndicator />
            <span className="local-chip" title="El audio y el texto se procesan en este equipo">
              <strong>Local</strong> Sin nube ni telemetría
            </span>
          </div>
        </div>
        <div className="meander" aria-hidden="true" />
      </header>

      <main className="app-main">{children}</main>

      <footer className="app-footer">
        <div className="app-footer-inner">
          <span>
            <strong className="footer-brand">Hermes</strong> — v0.1.0 · preview
          </span>
          <span>whisper.cpp + Ollama · se ejecuta en tu equipo, sin servicios externos</span>
        </div>
      </footer>
    </div>
  );
}
