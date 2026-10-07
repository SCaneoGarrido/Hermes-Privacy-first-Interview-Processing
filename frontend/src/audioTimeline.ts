// Sincronizacion entre el audio y los bloques de la transcripcion (ADR-023).

interface TimedBlock {
  start: number | null;
}

// Inicio efectivo de cada bloque. Los bloques sin `start` (creados al dividir
// un turno en el editor) heredan el del bloque anterior: se resaltan junto con
// el, en vez de quedar fuera del recorrido.
export function effectiveStarts(blocks: TimedBlock[]): (number | null)[] {
  let previous: number | null = null;
  return blocks.map((block) => {
    if (block.start != null) previous = block.start;
    return previous;
  });
}

// Bloques que se estan escuchando en `time` (segundos): el ultimo cuyo inicio
// efectivo ya paso, junto con los que heredaron ese mismo inicio (las mitades
// de un turno dividido en el editor se resaltan juntas). [first, last]
// inclusivo; first = -1 antes del primer bloque con tiempo.
export function activeBlockRange(blocks: TimedBlock[], time: number): { first: number; last: number } {
  const starts = effectiveStarts(blocks);
  let first = -1;
  let last = -1;
  for (let i = 0; i < starts.length; i++) {
    const start = starts[i];
    if (start == null) continue;
    if (start > time) break;
    if (first === -1 || start !== starts[first]) first = i;
    last = i;
  }
  return { first, last };
}

// Primer bloque del rango activo (para desplazar el panel hasta el).
export function activeBlockIndex(blocks: TimedBlock[], time: number): number {
  return activeBlockRange(blocks, time).first;
}

// 754.3 -> "12:34" (o "1:02:03" pasada la hora)
export function formatClock(seconds: number): string {
  const total = Math.max(0, Math.floor(seconds));
  const h = Math.floor(total / 3600);
  const m = Math.floor((total % 3600) / 60);
  const s = String(total % 60).padStart(2, "0");
  return h > 0 ? `${h}:${String(m).padStart(2, "0")}:${s}` : `${m}:${s}`;
}
