import { useState } from "react";
import { MAX_KEYWORD_LENGTH, MAX_KEYWORDS, parseKeywords, updateKeywords } from "../api/interviews";
import { ErrorBanner } from "./ErrorBanner";

// Ultimas palabras guardadas, para ofrecerlas en la siguiente entrevista.
// Solo es una comodidad de este navegador: si localStorage no esta
// disponible, la tarjeta funciona igual sin esa opcion.
const LAST_KEYWORDS_KEY = "hermes.lastKeywords";

function readLastKeywords(): string[] {
  try {
    const stored = localStorage.getItem(LAST_KEYWORDS_KEY);
    const parsed: unknown = stored ? JSON.parse(stored) : [];
    return Array.isArray(parsed) ? parsed.filter((k): k is string => typeof k === "string") : [];
  } catch {
    return [];
  }
}

function rememberKeywords(keywords: string[]) {
  try {
    localStorage.setItem(LAST_KEYWORDS_KEY, JSON.stringify(keywords));
  } catch {
    // Sin almacenamiento local: se pierde solo la sugerencia.
  }
}

function KeywordList({ keywords }: { keywords: string[] }) {
  return (
    <ul className="keyword-list">
      {keywords.map((keyword) => (
        <li key={keyword} className="keyword-tag">
          {keyword}
        </li>
      ))}
    </ul>
  );
}

interface KeywordsCardProps {
  interviewId: number;
  saved: string[];
  onSaved: () => void;
}

export function KeywordsCard({ interviewId, saved, onSaved }: KeywordsCardProps) {
  const [preview, setPreview] = useState<string[] | null>(null);
  const [fileProblem, setFileProblem] = useState<string | null>(null);
  const [error, setError] = useState<unknown>(null);
  const [saving, setSaving] = useState(false);
  const [lastKeywords] = useState(readLastKeywords);

  async function handleFile(file: File | undefined) {
    setPreview(null);
    setFileProblem(null);
    setError(null);
    if (!file) return;

    const keywords = parseKeywords(await file.text());
    if (keywords.length === 0) {
      setFileProblem("El archivo no tiene palabras. Escribilas separadas por coma, punto y coma o una por línea.");
    } else if (keywords.length > MAX_KEYWORDS) {
      setFileProblem(`El archivo tiene ${keywords.length} palabras; el máximo es ${MAX_KEYWORDS}.`);
    } else if (keywords.some((k) => k.length > MAX_KEYWORD_LENGTH)) {
      setFileProblem(`Hay términos de más de ${MAX_KEYWORD_LENGTH} caracteres. ¿Falta algún separador?`);
    } else {
      setPreview(keywords);
    }
  }

  async function save(keywords: string[]) {
    setError(null);
    setSaving(true);
    try {
      const result = await updateKeywords(interviewId, keywords);
      if (result.keywords.length > 0) rememberKeywords(result.keywords);
      setPreview(null);
      onSaved();
    } catch (err) {
      setError(err);
    } finally {
      setSaving(false);
    }
  }

  return (
    <div className="card">
      <div className="card-heading">
        <h2 className="section-title">Palabras clave</h2>
      </div>
      <p className="muted">
        Siglas, nombres y términos del tema de la entrevista (ej. SIGGES, GES, FONASA). Se usan para que la
        transcripción los escriba bien. Subí un archivo <code>.txt</code> con las palabras separadas por coma, punto y
        coma o una por línea. Se aplican la próxima vez que proceses la entrevista.
      </p>

      {saved.length > 0 ? (
        <>
          <p>{saved.length} palabras guardadas:</p>
          <KeywordList keywords={saved} />
          <button className="button-link-danger" onClick={() => save([])} disabled={saving}>
            Quitar palabras clave
          </button>
        </>
      ) : (
        <p className="muted">Esta entrevista no tiene palabras clave.</p>
      )}

      <div className="form form-inline">
        <input type="file" accept=".txt,text/plain" onChange={(e) => handleFile(e.target.files?.[0])} />
        {saved.length === 0 && lastKeywords.length > 0 && !preview && (
          <button onClick={() => save(lastKeywords)} disabled={saving}>
            Usar las de la última entrevista ({lastKeywords.length})
          </button>
        )}
      </div>

      {fileProblem && (
        <div className="banner banner-error" role="alert">
          <p>{fileProblem}</p>
        </div>
      )}

      {preview && (
        <>
          <p>Se van a guardar {preview.length} palabras:</p>
          <KeywordList keywords={preview} />
          <button onClick={() => save(preview)} disabled={saving}>
            {saving ? "Guardando…" : "Guardar palabras clave"}
          </button>
        </>
      )}

      <p className="muted">
        La transcripción usa como guía las primeras palabras que caben (~40); la corrección posterior con IA usa
        todas.
      </p>
      <ErrorBanner error={error} />
    </div>
  );
}
