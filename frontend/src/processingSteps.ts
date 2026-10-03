import type { ProcessingStep } from "./api/types";

// Orden real de los pasos en InterviewProcessingJobHandler/TranscriptEnhancer.
// Solo los dos primeros corren siempre: el glosario depende de que haya
// palabras clave, y correccion/anonimizacion/resumen de lo que se pidio al
// procesar.
export const STEP_ORDER: ProcessingStep[] = [
  "normalizando_audio",
  "transcribiendo",
  "aplicando_glosario",
  "corrigiendo_texto",
  "anonimizando",
  "generando_resumen",
];

export const STEP_INFO: Record<ProcessingStep, { label: string; hint: string }> = {
  normalizando_audio: { label: "Normalizando audio", hint: "Mono, 16 kHz" },
  transcribiendo: { label: "Transcribiendo con IA", hint: "whisper.cpp · el paso más largo" },
  aplicando_glosario: { label: "Aplicando palabras clave", hint: "Glosario de la entrevista" },
  corrigiendo_texto: { label: "Corrigiendo texto", hint: "Ortografía y turnos de habla" },
  anonimizando: { label: "Anonimizando", hint: "Nombres, lugares y organizaciones" },
  generando_resumen: { label: "Generando resumen", hint: "Documento aparte" },
};

export interface ProcessOptions {
  includeSummary: boolean;
  enhanceTranscript: boolean;
}

// La API no devuelve con que opciones se lanzo el job en curso. Este
// navegador recuerda las de los procesamientos que lanzo el mismo, solo para
// dibujar el recorrido con los pasos exactos; sin ese dato (otro navegador,
// almacenamiento bloqueado) los pasos opcionales se muestran como tales.
const OPTIONS_KEY_PREFIX = "hermes.processOptions.";

export function rememberProcessOptions(interviewId: number, options: ProcessOptions) {
  try {
    localStorage.setItem(OPTIONS_KEY_PREFIX + interviewId, JSON.stringify(options));
  } catch {
    // Sin almacenamiento local: el recorrido marca los pasos como opcionales.
  }
}

export function readProcessOptions(interviewId: number): ProcessOptions | null {
  try {
    const stored = localStorage.getItem(OPTIONS_KEY_PREFIX + interviewId);
    if (!stored) return null;
    const parsed = JSON.parse(stored) as Partial<ProcessOptions>;
    if (typeof parsed.includeSummary !== "boolean" || typeof parsed.enhanceTranscript !== "boolean") return null;
    return { includeSummary: parsed.includeSummary, enhanceTranscript: parsed.enhanceTranscript };
  } catch {
    return null;
  }
}

export type StepState = "done" | "current" | "pending" | "optional";

export interface StepView {
  step: ProcessingStep;
  label: string;
  hint: string;
  state: StepState;
}

// Arma el recorrido a mostrar: que pasos aplican y en que estado esta cada
// uno. "optional" = no se sabe si se pidio (ver readProcessOptions).
export function buildStepViews(
  currentStep: ProcessingStep | null,
  keywordCount: number,
  options: ProcessOptions | null
): StepView[] {
  const included = (step: ProcessingStep): boolean | null => {
    if (step === currentStep) return true;
    switch (step) {
      case "normalizando_audio":
      case "transcribiendo":
        return true;
      case "aplicando_glosario":
        return keywordCount > 0;
      case "corrigiendo_texto":
      case "anonimizando":
        if (currentStep === "corrigiendo_texto" || currentStep === "anonimizando") return true;
        return options ? options.enhanceTranscript : null;
      case "generando_resumen":
        return options ? options.includeSummary : null;
    }
  };

  const currentIndex = currentStep ? STEP_ORDER.indexOf(currentStep) : -1;

  return STEP_ORDER.flatMap((step, index) => {
    const isIncluded = included(step);
    if (isIncluded === false) return [];
    let state: StepState;
    if (isIncluded === null) state = "optional";
    else if (index < currentIndex) state = "done";
    else if (index === currentIndex) state = "current";
    else state = "pending";
    return [{ step, ...STEP_INFO[step], state }];
  });
}
