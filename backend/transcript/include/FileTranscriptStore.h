#pragma once

#include "ITranscriptStore.h"

#include <filesystem>
#include <string>

namespace hermes::transcript {

// Guarda las transcripciones como JSON junto al resto de los artefactos de la
// entrevista: <storageRoot>/interviews/<id>/transcript_segments.json (pipeline)
// y transcript_edited.json (usuario). Escritura atomica (archivo temporal +
// rename) para que un corte a mitad de escritura no deje un JSON roto.
class FileTranscriptStore : public ITranscriptStore {
    public:
        explicit FileTranscriptStore(std::string storageRoot);

        void savePipeline(int interviewId, const StructuredTranscript& transcript) override;
        void saveEdited(int interviewId, const StructuredTranscript& transcript) override;
        bool removeEdited(int interviewId) override;
        std::optional<LoadedTranscript> loadCurrent(int interviewId) override;
        bool hasEdits(int interviewId) override;

    private:
        std::filesystem::path interviewDir(int interviewId) const;

        std::string m_storageRoot;
};

}  // namespace hermes::transcript
