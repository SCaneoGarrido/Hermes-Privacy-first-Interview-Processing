// Isotipo: monograma "H" de proporciones epigraficas dentro de un sello, con
// el ala de las talarias de Hermes saliendo del asta derecha. Monocromo
// (currentColor + oro de acento) para que funcione chico y en ambos temas.
export function HermesMark({ size = 40 }: { size?: number }) {
  return (
    <svg width={size} height={size} viewBox="0 0 40 40" aria-hidden="true" focusable="false" className="hermes-mark">
      <circle cx="20" cy="20" r="18.5" fill="none" stroke="currentColor" strokeWidth="1.5" />
      <circle cx="20" cy="20" r="15.5" fill="none" stroke="var(--gold)" strokeWidth="0.75" opacity="0.7" />
      <path d="M13 12v16M23 12v16M13 20h10" fill="none" stroke="currentColor" strokeWidth="2.4" strokeLinecap="square" />
      <path
        d="M24 15c3-3.2 6.4-4.4 9.5-4.2-1.6 1-2.6 2-3.2 3.2 1.6-.4 3-.3 4.1.1-1.7.6-3 1.5-4 2.6 1.1 0 2.1.2 2.9.6-2.6.7-5.6.9-9.3-.5Z"
        fill="var(--gold)"
      />
    </svg>
  );
}
