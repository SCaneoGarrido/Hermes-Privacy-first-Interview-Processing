// Formateo para mostrar datos de la API a una persona (fechas, tamaños,
// duraciones). Solo presentacion: no cambia lo que se envia al backend.

const MONTHS = [
  "enero", "febrero", "marzo", "abril", "mayo", "junio",
  "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre",
];

// "2026-08-24 14:30:00" (MySQL DATETIME) -> "24 de agosto de 2026, 14:30 hs".
// Se parsea a mano para no pasar por la zona horaria del navegador: la fecha
// es la que cargo el investigador, no un instante a convertir.
export function formatInterviewDate(value: string): string {
  const match = /^(\d{4})-(\d{2})-(\d{2})(?:[ T](\d{2}):(\d{2}))?/.exec(value);
  if (!match) return value;
  const [, year, month, day, hour, minute] = match;
  const date = `${Number(day)} de ${MONTHS[Number(month) - 1]} de ${year}`;
  return hour ? `${date}, ${hour}:${minute} hs` : date;
}

// Clave para ordenar por fecha sin depender de Date (mismo formato MySQL).
export function dateSortKey(value: string): string {
  return value.replace("T", " ");
}

export function formatBytes(bytes: number): string {
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(0)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
}

// 1294 -> "21 min 34 s"
export function formatDuration(totalSeconds: number): string {
  const seconds = Math.round(totalSeconds);
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h} h ${m} min`;
  if (m > 0) return `${m} min ${s} s`;
  return `${s} s`;
}

// Cronometro: 81 -> "1:21", 3725 -> "1:02:05"
export function formatClock(totalSeconds: number): string {
  const h = Math.floor(totalSeconds / 3600);
  const m = Math.floor((totalSeconds % 3600) / 60);
  const s = totalSeconds % 60;
  const ss = String(s).padStart(2, "0");
  return h > 0 ? `${h}:${String(m).padStart(2, "0")}:${ss}` : `${m}:${ss}`;
}

export function fileNameFromPath(path: string): string {
  return path.split(/[\\/]/).pop() || path;
}

const ROMAN: [number, string][] = [[10, "X"], [9, "IX"], [5, "V"], [4, "IV"], [1, "I"]];

export function toRoman(n: number): string {
  let rest = n;
  let out = "";
  for (const [value, symbol] of ROMAN) {
    while (rest >= value) {
      out += symbol;
      rest -= value;
    }
  }
  return out;
}
