import { RefObject, useEffect, useState } from "react";

const SPEEDS = [0.75, 1, 1.25, 1.5];
const SKIP_SECONDS = 5;

interface TranscriptAudioPlayerProps {
  src: string;
  audioRef: RefObject<HTMLAudioElement>;
  // Tiempo actual de reproduccion (segundos), en cada timeupdate y al saltar.
  onTime: (seconds: number) => void;
  follow: boolean;
  onFollowChange: (follow: boolean) => void;
}

// Reproductor de la entrevista en la vista de lectura (ADR-023): <audio>
// nativo con saltos de 5 s, velocidad y "seguir el audio". preload="metadata":
// al abrir la pagina solo se piden los primeros bytes (duracion), no las
// decenas de MB del WAV; el backend responde por rangos.
//
// Atajos Alt+K (reproducir/pausar), Alt+J / Alt+L (5 s atras/adelante):
// funcionan mientras se escribe en el editor, y no chocan con Alt+flechas, que
// en el navegador es "volver atras" (sacaria al usuario de la edicion).
export function TranscriptAudioPlayer({ src, audioRef, onTime, follow, onFollowChange }: TranscriptAudioPlayerProps) {
  const [speed, setSpeed] = useState(1);
  const [unavailable, setUnavailable] = useState(false);

  const skip = (delta: number) => {
    const audio = audioRef.current;
    if (!audio) return;
    audio.currentTime = Math.max(0, Math.min(audio.duration || Infinity, audio.currentTime + delta));
    onTime(audio.currentTime);
  };

  const togglePlay = () => {
    const audio = audioRef.current;
    if (!audio) return;
    if (audio.paused) void audio.play();
    else audio.pause();
  };

  useEffect(() => {
    if (audioRef.current) audioRef.current.playbackRate = speed;
  }, [speed, audioRef]);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (!event.altKey || event.ctrlKey || event.metaKey) return;
      const key = event.key.toLowerCase();
      if (key === "k") togglePlay();
      else if (key === "j") skip(-SKIP_SECONDS);
      else if (key === "l") skip(SKIP_SECONDS);
      else return;
      event.preventDefault();
    };
    document.addEventListener("keydown", onKey);
    return () => document.removeEventListener("keydown", onKey);
  });

  if (unavailable) {
    return (
      <div className="audio-player audio-player-unavailable" role="status">
        El audio de esta entrevista no está disponible para escuchar (se genera al procesarla).
      </div>
    );
  }

  return (
    <div className="audio-player" role="region" aria-label="Reproductor de la entrevista">
      <audio
        ref={audioRef}
        src={src}
        preload="metadata"
        controls
        onTimeUpdate={(e) => onTime(e.currentTarget.currentTime)}
        onSeeked={(e) => onTime(e.currentTarget.currentTime)}
        onError={() => setUnavailable(true)}
      />
      <div className="audio-player-actions">
        <button type="button" className="editor-action" onClick={() => skip(-SKIP_SECONDS)} title="Alt+J">
          −5 s
        </button>
        <button type="button" className="editor-action" onClick={() => skip(SKIP_SECONDS)} title="Alt+L">
          +5 s
        </button>
        <label className="audio-player-speed">
          <span className="visually-hidden">Velocidad</span>
          <select value={speed} onChange={(e) => setSpeed(Number(e.target.value))}>
            {SPEEDS.map((s) => (
              <option key={s} value={s}>
                {s}×
              </option>
            ))}
          </select>
        </label>
        <label className="audio-player-follow">
          <input type="checkbox" checked={follow} onChange={(e) => onFollowChange(e.target.checked)} />
          Seguir el audio
        </label>
        <span className="audio-player-hint">Alt+K pausa · Alt+J / Alt+L ±5 s · clic en una marca de tiempo para saltar</span>
      </div>
    </div>
  );
}
