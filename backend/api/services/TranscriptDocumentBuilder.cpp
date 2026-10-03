#include "../include/services/TranscriptDocumentBuilder.h"

#include <array>
#include <sstream>
#include <string_view>

namespace {

// Prefijo que InterviewProcessingJobHandler antepone cuando la
// anonimizacion pedida no se pudo completar (nonAnonymizedNotice).
constexpr std::string_view NOTICE_PREFIX = "[AVISO HERMES]";

constexpr std::array<std::string_view, 2> SPEAKER_LABELS = {"Investigador", "Entrevistado"};

// Un parrafo de texto plano corta despues de MIN lineas si la ultima cierra
// una oracion, y siempre al llegar a MAX: bloques legibles sin depender de
// que whisper haya puntuado bien.
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

// "Investigador: hola" -> {"Investigador", "hola"}; nullopt si la linea no
// empieza con una etiqueta canonica.
std::optional<std::pair<std::string, std::string>> splitSpeaker(const std::string& line) {
    for (const auto label : SPEAKER_LABELS) {
        if (line.size() > label.size() && line.compare(0, label.size(), label) == 0 && line[label.size()] == ':') {
            return std::make_pair(std::string(label), trim(line.substr(label.size() + 1)));
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

}  // namespace

TranscriptDocument TranscriptDocumentBuilder::build(const std::string& transcriptText, const std::vector<double>& segmentStarts) {
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
        if (splitSpeaker(line)) ++labeled;
    }
    document.hasSpeakers = nonEmpty > 0 && labeled * 2 >= nonEmpty;

    if (document.hasSpeakers) {
        for (const auto& line : lines) {
            if (line.empty()) continue;
            auto speaker = splitSpeaker(line);
            if (speaker && (document.blocks.empty() || document.blocks.back().speaker != speaker->first)) {
                document.blocks.push_back({speaker->first, std::nullopt, speaker->second});
            } else if (speaker) {
                appendText(document.blocks.back().text, speaker->second);
            } else if (!document.blocks.empty()) {
                appendText(document.blocks.back().text, line);
            } else {
                document.blocks.push_back({std::nullopt, std::nullopt, line});
            }
        }
        return document;
    }

    // Texto plano: writePlainTranscript escribe una linea por segmento (aun
    // las vacias), asi que si las cantidades coinciden la linea i empieza en
    // segmentStarts[i].
    document.hasTimestamps = !segmentStarts.empty() && segmentStarts.size() == lines.size();

    TranscriptBlock current;
    size_t linesInParagraph = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) continue;

        if (linesInParagraph == 0 && document.hasTimestamps) {
            current.startSeconds = segmentStarts[i];
        }
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
