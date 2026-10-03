import { toRoman } from "../format";
import type { StepView } from "../processingSteps";
import { CheckIcon } from "./Icons";

const STATE_LABEL: Record<StepView["state"], string> = {
  done: "Completado",
  current: "En curso",
  pending: "Pendiente",
  optional: "Si se pidió",
};

// "Secuencia de umbrales": los pasos del procesamiento como hitos de un
// camino (hermai), numerados I..N. Sin porcentaje ni ETA: la API solo informa
// el paso actual.
export function ThresholdStepper({ steps }: { steps: StepView[] }) {
  return (
    <ol className="thresholds">
      {steps.map((step, index) => (
        <li
          key={step.step}
          className={`threshold threshold-${step.state}`}
          aria-current={step.state === "current" ? "step" : undefined}
        >
          <div className="threshold-top">
            <span className="threshold-numeral">{toRoman(index + 1)}</span>
            <span className="threshold-mark" aria-hidden="true">
              {step.state === "done" && <CheckIcon size={16} />}
              {step.state === "current" && <span className="spinner" />}
            </span>
          </div>
          <span className="threshold-state">{STATE_LABEL[step.state]}</span>
          <span className="threshold-label">{step.label}</span>
          <span className="threshold-hint">{step.hint}</span>
        </li>
      ))}
    </ol>
  );
}
