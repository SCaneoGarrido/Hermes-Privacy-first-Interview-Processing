import { DragEvent, useRef, useState } from "react";
import { MAX_UPLOAD_BYTES } from "../api/interviews";
import { formatBytes } from "../format";
import { UploadIcon } from "./Icons";

// Mismos formatos que acepta FileFormatGuard en el backend (que igual vuelve
// a validar por firma de bytes); aca es solo para avisar antes de subir.
const ACCEPTED_EXTENSIONS = ["mp3", "wav", "ogg", "m4a", "mp4"];
const ACCEPT_ATTR = "audio/mpeg,audio/wav,audio/ogg,audio/x-m4a,video/mp4,.mp3,.wav,.ogg,.m4a,.mp4";

interface DropzoneProps {
  file: File | null;
  onFileChange: (file: File | null) => void;
  compact?: boolean;
}

function validate(file: File): string | null {
  const extension = file.name.split(".").pop()?.toLowerCase() ?? "";
  if (!ACCEPTED_EXTENSIONS.includes(extension)) {
    return `El formato .${extension} no está soportado. Usá mp3, wav, ogg, m4a o mp4.`;
  }
  if (file.size > MAX_UPLOAD_BYTES) {
    return `El archivo pesa ${formatBytes(file.size)}; el máximo es 1 GB.`;
  }
  return null;
}

export function Dropzone({ file, onFileChange, compact }: DropzoneProps) {
  const inputRef = useRef<HTMLInputElement>(null);
  const [dragging, setDragging] = useState(false);
  const [problem, setProblem] = useState<string | null>(null);

  function pick(candidate: File | undefined) {
    if (!candidate) return;
    const error = validate(candidate);
    setProblem(error);
    onFileChange(error ? null : candidate);
  }

  function handleDrop(event: DragEvent) {
    event.preventDefault();
    setDragging(false);
    pick(event.dataTransfer.files?.[0]);
  }

  return (
    <div>
      <div
        className={`dropzone${dragging ? " dropzone-active" : ""}${compact ? " dropzone-compact" : ""}`}
        onDragOver={(e) => {
          e.preventDefault();
          setDragging(true);
        }}
        onDragLeave={() => setDragging(false)}
        onDrop={handleDrop}
      >
        <UploadIcon size={compact ? 20 : 28} />
        {file ? (
          <p>
            <strong>{file.name}</strong> · {formatBytes(file.size)}
          </p>
        ) : (
          <p>Arrastrá el audio o video acá, o</p>
        )}
        <button type="button" className="btn btn-secondary" onClick={() => inputRef.current?.click()}>
          {file ? "Elegir otro archivo" : "Elegir archivo"}
        </button>
        <input
          ref={inputRef}
          type="file"
          accept={ACCEPT_ATTR}
          className="visually-hidden"
          tabIndex={-1}
          onChange={(e) => {
            pick(e.target.files?.[0]);
            e.target.value = "";
          }}
        />
        <p className="dropzone-hint">
          mp3, wav, ogg, m4a o video mp4 · máx. 1 GB. De un video solo se usa el audio; el video original se borra al
          procesar.
        </p>
      </div>
      {problem && (
        <div className="banner banner-error" role="alert">
          <p>{problem}</p>
        </div>
      )}
    </div>
  );
}
