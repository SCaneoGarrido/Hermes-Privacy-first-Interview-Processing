#include "../include/services/TranscriptDocumentBuilder.h"

#include <array>
#include <sstream>
#include <string_view>

namespace {

using hermes::transcript::SpeakerSource;
using hermes::transcript::StructuredTranscript;
using hermes::transcript::TranscriptTurn;

// Prefijo del aviso de no anonimizacion (InterviewProcessingJobHandler).
constexpr std::string_view NOTICE_PREFIX = "[AVISO HERMES]";

// Etiquetas que escribia el LLM antes de ADR-022, con su rol equivalente.
constexpr std::array<std::pair<std::string_view, std::string_view>, 2> LEGACY_SPEAKER_LABELS = {{
    {"Investigador", "interviewer"},
    {"Entrevistado", "subject"},
}};

// Un parrafo de texto plano corta despues de MIN segmentos si el ultimo
// cierra una oracion, y siempre al llegar a MAX: bloques legibles sin
// depender de que whisper haya puntuado bien.
constexpr size_t PARAGRAPH_MIN_LINES = 4;
constexpr size_t PARAGRAPH_MAX_LINES = 9;

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(trim(line));
    }
    return lines;
}

// "Investigador: hola" -> {"interviewer", "hola"}; nullopt si la linea no
// empieza con una etiqueta conocida.
std::optional<std::pair<std::string, std::string>> splitLegacySpeaker(const std::string& line) {
    for (const auto& [label, key] : LEGACY_SPEAKER_LABELS) {
        if (line.size() > label.size() && line.compare(0, label.size(), label) == 0 && line[label.size()] == ':') {
            return std::make_pair(std::string(key), trim(line.substr(label.size() + 1)));
        }
    }
    return std::nullopt;
}

bool endsSentence(const std::string& line) {
    if (line.empty()) return false;
    const char last = line.back();
    return last == '.' || last == '?' || last == '!' || last == ':';
}

void appendText(std::string& target, const std::string& addition) {
    if (addition.empty()) return;
    if (!target.empty()) target += ' ';
    target += addition;
}

std::optional<double> toSeconds(const std::optional<int64_t>& ms) {
    if (!ms) return std::nullopt;
    return static_cast<double>(*ms) / 1000.0;
}

std::optional<std::string> roleKey(const TranscriptTurn& turn) {
    if (!turn.speaker) return std::nullopt;
    return std::string(hermes::transcript::speakerRoleKey(*turn.speaker));
}

}  // namespace

TranscriptDocument TranscriptDocumentBuilder::build(const StructuredTranscript& transcript) {
    TranscriptDocument document;
    document.notice = transcript.notice;
    document.hasSpeakers = transcript.hasSpeakers();
    document.speakerSource = std::string(hermes::transcript::speakerSourceKey(transcript.speakerSource));

    bool anyTimestamp = false;
    for (const auto& turn : transcript.turns) {
        if (turn.startMs) {
            anyTimestamp = true;
            break;
        }
    }
    document.hasTimestamps = anyTimestamp;

    // Version editada a mano: cada turno es un bloque tal como lo dejo el
    // usuario (si dividio un turno, no se vuelve a unir).
    if (transcript.speakerSource == SpeakerSource::Manual) {
        for (const auto& turn : transcript.turns) {
            const std::string text = trim(turn.text);
            if (text.empty()) continue;
            document.blocks.push_back({roleKey(turn), toSeconds(turn.startMs), toSeconds(turn.endMs), text});
        }
        return document;
    }

    if (document.hasSpeakers) {
        for (const auto& turn : transcript.turns) {
            const std::string text = trim(turn.text);
            if (text.empty()) continue;
            const auto speaker = roleKey(turn);
            if (!document.blocks.empty() && document.blocks.back().speaker == speaker) {
                appendText(document.blocks.back().text, text);
                if (turn.endMs) document.blocks.back().endSeconds = toSeconds(turn.endMs);
                continue;
            }
            document.blocks.push_back({speaker, toSeconds(turn.startMs), toSeconds(turn.endMs), text});
        }
        return document;
    }

    TranscriptBlock current;
    size_t linesInParagraph = 0;
    for (const auto& turn : transcript.turns) {
        const std::string text = trim(turn.text);
        if (text.empty()) continue;
        if (linesInParagraph == 0) {
            current.startSeconds = toSeconds(turn.startMs);
        }
        if (turn.endMs) current.endSeconds = toSeconds(turn.endMs);
        appendText(current.text, text);
        ++linesInParagraph;

        if ((linesInParagraph >= PARAGRAPH_MIN_LINES && endsSentence(text)) || linesInParagraph >= PARAGRAPH_MAX_LINES) {
            document.blocks.push_back(std::move(current));
            current = TranscriptBlock{};
            linesInParagraph = 0;
        }
    }
    if (linesInParagraph > 0) {
        document.blocks.push_back(std::move(current));
    }
    return document;
}

TranscriptDocument TranscriptDocumentBuilder::buildLegacy(const std::string& transcriptText) {
    TranscriptDocument document;

    std::string body = transcriptText;
    if (body.compare(0, NOTICE_PREFIX.size(), NOTICE_PREFIX) == 0) {
        const auto end = body.find("\n\n");
        document.notice = trim(body.substr(0, end));
        body = end == std::string::npos ? "" : body.substr(end + 2);
    }

    const std::vector<std::string> lines = splitLines(body);

    size_t labeled = 0;
    size_t nonEmpty = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        ++nonEmpty;
        if (splitLegacySpeaker(line)) ++labeled;
    }
    document.hasSpeakers = nonEmpty > 0 && labeled * 2 >= nonEmpty;

    if (document.hasSpeakers) {
        document.speakerSource = "llm";
        for (const auto& line : lines) {
            if (line.empty()) continue;
            auto speaker = splitLegacySpeaker(line);
            if (speaker && (document.blocks.empty() || document.blocks.back().speaker != speaker->first)) {
                document.blocks.push_back({speaker->first, std::nullopt, std::nullopt, speaker->second});
            } else if (speaker) {
                appendText(document.blocks.back().text, speaker->second);
            } else if (!document.blocks.empty()) {
                appendText(document.blocks.back().text, line);
            } else {
                document.blocks.push_back({std::nullopt, std::nullopt, std::nullopt, line});
            }
        }
        return document;
    }

    TranscriptBlock current;
    size_t linesInParagraph = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        appendText(current.text, line);
        ++linesInParagraph;
        if ((linesInParagraph >= PARAGRAPH_MIN_LINES && endsSentence(line)) || linesInParagraph >= PARAGRAPH_MAX_LINES) {
            document.blocks.push_back(std::move(current));
            current = TranscriptBlock{};
            linesInParagraph = 0;
        }
    }
    if (linesInParagraph > 0) {
        document.blocks.push_back(std::move(current));
    }
    return document;
}
