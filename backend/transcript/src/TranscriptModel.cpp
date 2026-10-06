#include "../include/TranscriptModel.h"

#include <algorithm>

namespace hermes::transcript {

namespace {

constexpr std::string_view INTERVIEWER_LABEL = "Investigador";
constexpr std::string_view DEFAULT_SUBJECT_LABEL = "Entrevistado";

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

}  // namespace

std::string_view speakerRoleKey(SpeakerRole role) {
    return role == SpeakerRole::Interviewer ? "interviewer" : "subject";
}

std::optional<SpeakerRole> parseSpeakerRole(std::string_view key) {
    if (key == "interviewer") return SpeakerRole::Interviewer;
    if (key == "subject") return SpeakerRole::Subject;
    return std::nullopt;
}

std::string_view speakerSourceKey(SpeakerSource source) {
    switch (source) {
        case SpeakerSource::Diarization: return "diarization";
        case SpeakerSource::Manual: return "manual";
        case SpeakerSource::None: break;
    }
    return "none";
}

SpeakerSource parseSpeakerSource(std::string_view key) {
    if (key == "diarization") return SpeakerSource::Diarization;
    if (key == "manual") return SpeakerSource::Manual;
    return SpeakerSource::None;
}

bool StructuredTranscript::hasSpeakers() const {
    return std::any_of(turns.begin(), turns.end(), [](const TranscriptTurn& turn) { return turn.speaker.has_value(); });
}

std::string SpeakerLabels::labelFor(const std::optional<SpeakerRole>& role) const {
    if (!role) return "";
    return *role == SpeakerRole::Interviewer ? interviewer : subject;
}

SpeakerLabels makeSpeakerLabels(const std::string& subjectType) {
    const std::string subject = trim(subjectType);
    return {std::string(INTERVIEWER_LABEL), subject.empty() ? std::string(DEFAULT_SUBJECT_LABEL) : subject};
}

}  // namespace hermes::transcript
