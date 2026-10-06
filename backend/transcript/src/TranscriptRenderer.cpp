#include "../include/TranscriptRenderer.h"

namespace hermes::transcript {

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

// Los saltos de linea dentro de un turno (los puede escribir el usuario en el
// editor) romperian el formato de una linea por turno.
std::string singleLine(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        out += (c == '\n' || c == '\r') ? ' ' : c;
    }
    return trim(out);
}

}  // namespace

std::string renderPlainText(const StructuredTranscript& transcript, const SpeakerLabels& labels, bool includeNotice) {
    std::string out;
    if (includeNotice && transcript.notice && !transcript.notice->empty()) {
        out += *transcript.notice + "\n\n";
    }

    if (!transcript.hasSpeakers()) {
        for (const auto& turn : transcript.turns) {
            out += singleLine(turn.text) + "\n";
        }
        return out;
    }

    std::string currentLine;
    std::optional<SpeakerRole> currentSpeaker;
    bool open = false;
    auto flush = [&]() {
        if (!open) return;
        const std::string label = labels.labelFor(currentSpeaker);
        out += (label.empty() ? "" : label + ": ") + currentLine + "\n";
        currentLine.clear();
        open = false;
    };

    for (const auto& turn : transcript.turns) {
        const std::string text = singleLine(turn.text);
        if (text.empty()) continue;
        if (open && turn.speaker == currentSpeaker) {
            currentLine += " " + text;
            continue;
        }
        flush();
        currentSpeaker = turn.speaker;
        currentLine = text;
        open = true;
    }
    flush();
    return out;
}

}  // namespace hermes::transcript
