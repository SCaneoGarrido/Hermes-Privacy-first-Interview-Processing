#include "../include/FileTranscriptStore.h"
#include "../../api/include/logger.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <system_error>

namespace hermes::transcript {

namespace {

constexpr const char* PIPELINE_FILE = "transcript_segments.json";
constexpr const char* EDITED_FILE = "transcript_edited.json";

nlohmann::json optionalInt(const std::optional<int64_t>& value) {
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

nlohmann::json toJson(const StructuredTranscript& transcript) {
    nlohmann::json turns = nlohmann::json::array();
    for (const auto& turn : transcript.turns) {
        turns.push_back({
            {"start_ms", optionalInt(turn.startMs)},
            {"end_ms", optionalInt(turn.endMs)},
            {"speaker", turn.speaker ? nlohmann::json(std::string(speakerRoleKey(*turn.speaker))) : nlohmann::json(nullptr)},
            {"text", turn.text},
        });
    }
    return {
        {"version", transcript.version},
        {"speaker_source", std::string(speakerSourceKey(transcript.speakerSource))},
        {"notice", transcript.notice ? nlohmann::json(*transcript.notice) : nlohmann::json(nullptr)},
        {"turns", std::move(turns)},
    };
}

std::optional<int64_t> readOptionalInt(const nlohmann::json& object, const char* key) {
    const auto it = object.find(key);
    if (it == object.end() || it->is_null()) return std::nullopt;
    return it->get<int64_t>();
}

StructuredTranscript fromJson(const nlohmann::json& json) {
    StructuredTranscript transcript;
    transcript.version = json.value("version", 1);
    transcript.speakerSource = parseSpeakerSource(json.value("speaker_source", std::string("none")));
    if (json.contains("notice") && json["notice"].is_string()) {
        transcript.notice = json["notice"].get<std::string>();
    }
    for (const auto& item : json.at("turns")) {
        TranscriptTurn turn;
        turn.startMs = readOptionalInt(item, "start_ms");
        turn.endMs = readOptionalInt(item, "end_ms");
        if (item.contains("speaker") && item["speaker"].is_string()) {
            turn.speaker = parseSpeakerRole(item["speaker"].get<std::string>());
        }
        turn.text = item.value("text", std::string());
        transcript.turns.push_back(std::move(turn));
    }
    return transcript;
}

void writeAtomically(const std::filesystem::path& path, const StructuredTranscript& transcript) {
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::path tmp = path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("No se pudo escribir " + tmp.string());
        }
        // error_handler_t::replace: misma red de seguridad que
        // transcript_raw.json ante UTF-8 invalido de whisper.
        out << toJson(transcript).dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
        if (!out.good()) {
            throw std::runtime_error("Error escribiendo " + tmp.string());
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        throw std::runtime_error("No se pudo reemplazar " + path.string());
    }
}

std::optional<StructuredTranscript> readTranscript(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return std::nullopt;
    try {
        return fromJson(nlohmann::json::parse(in));
    } catch (const std::exception& e) {
        log_event("[FileTranscriptStore][readTranscript] No se pudo leer " + path.string() + ": " + e.what());
        return std::nullopt;
    }
}

}  // namespace

FileTranscriptStore::FileTranscriptStore(std::string storageRoot) : m_storageRoot(std::move(storageRoot)) {}

std::filesystem::path FileTranscriptStore::interviewDir(int interviewId) const {
    return std::filesystem::path(m_storageRoot) / "interviews" / std::to_string(interviewId);
}

void FileTranscriptStore::savePipeline(int interviewId, const StructuredTranscript& transcript) {
    writeAtomically(interviewDir(interviewId) / PIPELINE_FILE, transcript);
}

void FileTranscriptStore::saveEdited(int interviewId, const StructuredTranscript& transcript) {
    writeAtomically(interviewDir(interviewId) / EDITED_FILE, transcript);
}

bool FileTranscriptStore::removeEdited(int interviewId) {
    std::error_code ec;
    std::filesystem::remove(interviewDir(interviewId) / EDITED_FILE, ec);
    if (ec) {
        log_event("[FileTranscriptStore][removeEdited] No se pudo borrar la version editada (interview_id=" +
                  std::to_string(interviewId) + "): " + ec.message());
        return false;
    }
    return true;
}

std::optional<LoadedTranscript> FileTranscriptStore::loadCurrent(int interviewId) {
    if (auto edited = readTranscript(interviewDir(interviewId) / EDITED_FILE)) {
        return LoadedTranscript{std::move(*edited), true};
    }
    if (auto pipeline = readTranscript(interviewDir(interviewId) / PIPELINE_FILE)) {
        return LoadedTranscript{std::move(*pipeline), false};
    }
    return std::nullopt;
}

bool FileTranscriptStore::hasEdits(int interviewId) {
    std::error_code ec;
    return std::filesystem::exists(interviewDir(interviewId) / EDITED_FILE, ec);
}

}  // namespace hermes::transcript
