#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hermes::transcript {

// Rol de quien habla en una entrevista de investigacion. Se guarda el rol y
// no el nombre visible: el nombre del sujeto sale de interviews.subject_type
// al momento de leer (ver SpeakerLabels), asi renombrarlo no obliga a
// reprocesar ni a reescribir archivos (ADR-022).
enum class SpeakerRole { Interviewer, Subject };

// Clave estable en JSON/API: "interviewer" / "subject".
std::string_view speakerRoleKey(SpeakerRole role);
std::optional<SpeakerRole> parseSpeakerRole(std::string_view key);

// De donde salio la atribucion de hablantes de una transcripcion.
enum class SpeakerSource { None, Diarization, Manual };

std::string_view speakerSourceKey(SpeakerSource source);
SpeakerSource parseSpeakerSource(std::string_view key);

// Un turno (o fragmento de turno) de la transcripcion. Los tiempos son ms del
// audio original; pueden faltar en turnos creados a mano en el editor (ej. al
// dividir un turno no se sabe en que momento exacto empieza la segunda mitad).
struct TranscriptTurn {
    std::optional<int64_t> startMs;
    std::optional<int64_t> endMs;
    std::optional<SpeakerRole> speaker;
    std::string text;
};

// Fuente de verdad de la transcripcion entregada (transcript_segments.json /
// transcript_edited.json). El .txt de descarga y la vista de lectura se
// derivan de esto.
struct StructuredTranscript {
    int version = 1;
    SpeakerSource speakerSource = SpeakerSource::None;
    // "[AVISO HERMES] ..." cuando se pidio anonimizar y no se pudo: el texto
    // contiene datos personales y quien lo lea tiene que saberlo.
    std::optional<std::string> notice;
    std::vector<TranscriptTurn> turns;

    bool hasSpeakers() const;
};

// Nombres visibles de cada rol: "Investigador" y el tipo de sujeto que el
// usuario cargo al crear la entrevista (ej. "Monitor GES").
struct SpeakerLabels {
    std::string interviewer;
    std::string subject;

    // Etiqueta del rol, o vacio si el turno no tiene hablante asignado.
    std::string labelFor(const std::optional<SpeakerRole>& role) const;
};

// subjectType vacio (no deberia pasar: es obligatorio al crear la
// entrevista) cae a "Entrevistado".
SpeakerLabels makeSpeakerLabels(const std::string& subjectType);

}  // namespace hermes::transcript
