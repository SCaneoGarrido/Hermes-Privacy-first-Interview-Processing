import { useEffect, useLayoutEffect, useMemo, useRef, useState } from "react";
import type { SpeakerKey, TranscriptBlock, TranscriptDocument } from "../api/types";
import { activeBlockRange, effectiveStarts, formatClock } from "../audioTimeline";
import { ConfirmDialog } from "./ConfirmDialog";

interface DraftBlock extends TranscriptBlock {
  id: number; // clave estable para React mientras se divide/une/borra
}

interface PendingConfirm {
  title: string;
  message: string;
  confirmLabel: string;
  action: () => void;
}

interface TranscriptEditorProps {
  transcript: TranscriptDocument;
  saving: boolean;
  onSave: (blocks: TranscriptBlock[]) => void;
  onCancel: () => void;
  onRestore: () => void;
  onDirtyChange: (dirty: boolean) => void;
  // Reproduccion sincronizada (ADR-023): tiempo actual del audio (null si no
  // hay audio/tiempos), si hay que seguirlo, y saltar a un punto.
  playbackTime: number | null;
  followAudio: boolean;
  onSeek: (seconds: number) => void;
}

let nextBlockId = 1;

function toDraft(blocks: TranscriptBlock[]): DraftBlock[] {
  return blocks.map((block) => ({ ...block, id: nextBlockId++ }));
}

function stripIds(blocks: DraftBlock[]): TranscriptBlock[] {
  return blocks.map(({ speaker, start, end, text }) => ({ speaker, start, end, text }));
}

function autoGrow(textarea: HTMLTextAreaElement | null) {
  if (!textarea) return;
  textarea.style.height = "auto";
  textarea.style.height = `${textarea.scrollHeight}px`;
}

// Edicion de la transcripcion en la vista de lectura: texto y hablante de cada
// turno, dividir/unir/eliminar turnos e intercambiar los dos hablantes de toda
// la entrevista (el arreglo de un clic cuando la asignacion automatica de
// roles salio al reves). Guarda una version editada; la original del
// pipeline se conserva en el servidor y se puede restaurar.
export function TranscriptEditor({
  transcript,
  saving,
  onSave,
  onCancel,
  onRestore,
  onDirtyChange,
  playbackTime,
  followAudio,
  onSeek,
}: TranscriptEditorProps) {
  const [draft, setDraft] = useState<DraftBlock[]>(() => toDraft(transcript.blocks));
  const [confirm, setConfirm] = useState<PendingConfirm | null>(null);
  const textareas = useRef(new Map<number, HTMLTextAreaElement>());
  const blockItems = useRef(new Map<number, HTMLLIElement>());

  // Bloque que se esta escuchando, sobre el borrador (puede tener bloques
  // divididos sin tiempo propio: heredan el del anterior).
  const starts = useMemo(() => effectiveStarts(draft), [draft]);
  const activeRange = useMemo(
    () => (playbackTime == null ? { first: -1, last: -1 } : activeBlockRange(draft, playbackTime)),
    [draft, playbackTime]
  );
  const activeId = activeRange.first >= 0 ? draft[activeRange.first]?.id : undefined;
  const isActive = (index: number) => activeRange.first >= 0 && index >= activeRange.first && index <= activeRange.last;

  // Seguir el audio, salvo mientras se escribe en un bloque: desplazar la
  // pagina en ese momento le sacaria el texto de debajo del cursor.
  useEffect(() => {
    if (!followAudio || activeId === undefined) return;
    if (document.activeElement instanceof HTMLTextAreaElement) return;
    blockItems.current.get(activeId)?.scrollIntoView({ block: "nearest", behavior: "smooth" });
  }, [activeId, followAudio]);

  const original = useMemo(() => JSON.stringify(transcript.blocks), [transcript.blocks]);
  const dirty = useMemo(() => JSON.stringify(stripIds(draft)) !== original, [draft, original]);

  useEffect(() => {
    onDirtyChange(dirty);
  }, [dirty, onDirtyChange]);

  // Si se guardo (o restauro) y el documento cambio, el borrador arranca de nuevo.
  useEffect(() => {
    setDraft(toDraft(transcript.blocks));
  }, [transcript.blocks]);

  useLayoutEffect(() => {
    textareas.current.forEach(autoGrow);
  }, [draft.length]);

  const labelFor = (key: SpeakerKey | null) =>
    transcript.speakers.find((s) => s.key === key)?.label ?? "Sin asignar";

  const update = (id: number, change: Partial<TranscriptBlock>) =>
    setDraft((blocks) => blocks.map((b) => (b.id === id ? { ...b, ...change } : b)));

  const split = (index: number) => {
    const block = draft[index];
    const textarea = textareas.current.get(block.id);
    const cursor = textarea?.selectionStart ?? 0;
    const before = block.text.slice(0, cursor).trim();
    const after = block.text.slice(cursor).trim();
    if (!before || !after) return;
    // No se sabe en que segundo exacto empieza la segunda mitad.
    const first: DraftBlock = { ...block, text: before, end: null };
    const second: DraftBlock = { ...block, id: nextBlockId++, text: after, start: null };
    setDraft((blocks) => [...blocks.slice(0, index), first, second, ...blocks.slice(index + 1)]);
  };

  const mergeWithPrevious = (index: number) => {
    if (index === 0) return;
    setDraft((blocks) => {
      const prev = blocks[index - 1];
      const cur = blocks[index];
      const merged: DraftBlock = {
        ...prev,
        text: `${prev.text.trim()} ${cur.text.trim()}`.trim(),
        end: cur.end ?? prev.end,
      };
      return [...blocks.slice(0, index - 1), merged, ...blocks.slice(index + 1)];
    });
  };

  const remove = (index: number) => {
    const block = draft[index];
    const doRemove = () => setDraft((blocks) => blocks.filter((b) => b.id !== block.id));
    if (!block.text.trim()) {
      doRemove();
      return;
    }
    setConfirm({
      title: "Eliminar turno",
      message: `Se quitará este turno de ${labelFor(block.speaker)} de la transcripción editada. La versión original se conserva y se puede restaurar.`,
      confirmLabel: "Eliminar turno",
      action: doRemove,
    });
  };

  const swapSpeakers = () =>
    setDraft((blocks) =>
      blocks.map((b) => ({
        ...b,
        speaker: b.speaker === "interviewer" ? "subject" : b.speaker === "subject" ? "interviewer" : null,
      }))
    );

  const requestCancel = () => {
    if (!dirty) {
      onCancel();
      return;
    }
    setConfirm({
      title: "Descartar cambios",
      message: "Los cambios sin guardar se van a perder.",
      confirmLabel: "Descartar",
      action: onCancel,
    });
  };

  const requestRestore = () =>
    setConfirm({
      title: "Restaurar la transcripción original",
      message: "Se descartan todas las ediciones guardadas y se vuelve a la versión que generó el procesamiento.",
      confirmLabel: "Restaurar original",
      action: onRestore,
    });

  const [interviewer, subject] = transcript.speakers;

  return (
    <div className="transcript-editor">
      <div className="editor-bar" role="toolbar" aria-label="Edición de la transcripción">
        <p className="editor-hint">
          Editá el texto y el hablante de cada turno. Para dividir un turno, ubicá el cursor donde empieza el otro
          hablante y usá <strong>Dividir</strong>.
        </p>
        <div className="editor-bar-actions">
          {interviewer && subject && (
            <button type="button" className="btn btn-secondary" onClick={swapSpeakers} disabled={saving}>
              Intercambiar {interviewer.label} ↔ {subject.label}
            </button>
          )}
          {transcript.edited && (
            <button type="button" className="btn btn-secondary" onClick={requestRestore} disabled={saving}>
              Restaurar original
            </button>
          )}
          <button type="button" className="btn btn-secondary" onClick={requestCancel} disabled={saving}>
            Cancelar
          </button>
          <button
            type="button"
            className="btn btn-primary"
            onClick={() => onSave(stripIds(draft))}
            disabled={saving || !dirty}
          >
            {saving ? "Guardando…" : "Guardar cambios"}
          </button>
        </div>
      </div>

      <ol className="editor-blocks">
        {draft.map((block, index) => (
          <li
            key={block.id}
            ref={(el) => {
              if (el) blockItems.current.set(block.id, el);
              else blockItems.current.delete(block.id);
            }}
            className={`editor-block${block.speaker ? ` turn-${block.speaker}` : ""}${
              isActive(index) ? " editor-block-active" : ""
            }`}
          >
            <div className="editor-block-head">
              <label className="editor-speaker">
                <span className="visually-hidden">Hablante del turno {index + 1}</span>
                <select
                  value={block.speaker ?? ""}
                  onChange={(e) => update(block.id, { speaker: (e.target.value || null) as SpeakerKey | null })}
                  disabled={saving}
                >
                  {transcript.speakers.map((s) => (
                    <option key={s.key} value={s.key}>
                      {s.label}
                    </option>
                  ))}
                  <option value="">Sin asignar</option>
                </select>
              </label>
              {block.start != null && (
                <button
                  type="button"
                  className="editor-time turn-seek"
                  onClick={() => {
                    const start = starts[index];
                    if (start != null) onSeek(start);
                  }}
                  disabled={playbackTime == null}
                  title={playbackTime == null ? undefined : "Escuchar desde aquí"}
                >
                  {formatClock(block.start)}
                </button>
              )}
              <div className="editor-block-actions">
                <button
                  type="button"
                  className="editor-action"
                  // Mantener el foco (y el cursor) en el texto al hacer clic.
                  onMouseDown={(e) => e.preventDefault()}
                  onClick={() => split(index)}
                  disabled={saving}
                  title="Divide el turno en la posición del cursor"
                >
                  Dividir
                </button>
                <button
                  type="button"
                  className="editor-action"
                  onClick={() => mergeWithPrevious(index)}
                  disabled={saving || index === 0}
                  title="Une este turno al final del anterior"
                >
                  Unir con anterior
                </button>
                <button
                  type="button"
                  className="editor-action editor-action-danger"
                  onClick={() => remove(index)}
                  disabled={saving}
                >
                  Eliminar
                </button>
              </div>
            </div>
            <textarea
              ref={(el) => {
                if (el) textareas.current.set(block.id, el);
                else textareas.current.delete(block.id);
              }}
              className="editor-text"
              value={block.text}
              onChange={(e) => {
                update(block.id, { text: e.target.value });
                autoGrow(e.target);
              }}
              disabled={saving}
              rows={2}
              aria-label={`Texto del turno ${index + 1} (${labelFor(block.speaker)})`}
            />
          </li>
        ))}
      </ol>

      {confirm && (
        <ConfirmDialog
          title={confirm.title}
          message={confirm.message}
          confirmLabel={confirm.confirmLabel}
          onConfirm={() => {
            confirm.action();
            setConfirm(null);
          }}
          onCancel={() => setConfirm(null)}
        />
      )}
    </div>
  );
}
