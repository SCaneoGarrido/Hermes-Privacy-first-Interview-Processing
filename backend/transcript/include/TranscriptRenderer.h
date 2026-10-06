#pragma once

#include "TranscriptModel.h"

#include <string>

namespace hermes::transcript {

// Convierte la transcripcion estructurada en texto plano. Pura: sin I/O.
//
// - Con hablantes: una linea por turno, "Investigador: ..." / "Monitor GES: ...",
//   uniendo turnos consecutivos del mismo hablante. Turnos sin hablante van
//   sin etiqueta.
// - Sin hablantes: una linea por turno (= segmento de whisper), como el
//   transcript_final.txt historico.
// Si hay aviso de no anonimizacion y includeNotice, va primero, separado por
// una linea en blanco (formato que TranscriptDocumentBuilder sabe separar).
std::string renderPlainText(const StructuredTranscript& transcript, const SpeakerLabels& labels, bool includeNotice = true);

}  // namespace hermes::transcript
