#pragma once

#include <filesystem>

#include "IJobHandler.h"
#include "IInterviewJobRepository.h"
#include "../../api/include/repositories/IInterviewRepository.h"
#include "../../audio/include/IAudioNormalizer.h"
#include "../../diarization/include/IDiarizer.h"
#include "../../transcript/include/ITranscriptStore.h"
#include "../../transcription/include/ITranscriber.h"
#include "../../llm/include/TranscriptEnhancer.h"
#include "../../llm/include/GlossarySanitizer.h"

namespace hermes::jobs {

// Orquesta el pipeline completo de procesamiento: normaliza el audio,
// transcribe con whisper.cpp (guarda el JSON crudo en
// interview_jobs.raw_transcript_path), identifica quien habla con
// diarizacion acustica (investigador / sujeto, ver ADR-021), aplica el
// glosario y guarda la transcripcion estructurada (ITranscriptStore) + un TXT
// derivado como resultado utilizable de base. Despues, solo si se pidio,
// intenta mejorarla con Ollama (correccion, anonimizacion, resumen). Ni la
// diarizacion ni Ollama tumban el job si fallan: se conserva lo ya obtenido.
//
// No toca DatabaseManager, whisper.cpp/FFmpeg/sherpa-onnx, ni Ollama
// directamente: todo pasa por las interfaces inyectadas (Filosofia de
// Repositorios).
class InterviewProcessingJobHandler : public IJobHandler {
    public:
        InterviewProcessingJobHandler(IInterviewRepository& interviewRepository,
                                       IInterviewJobRepository& jobRepository,
                                       hermes::audio::IAudioNormalizer& audioNormalizer,
                                       hermes::transcription::ITranscriber& transcriber,
                                       hermes::diarization::IDiarizer& diarizer,
                                       hermes::transcript::ITranscriptStore& transcriptStore,
                                       hermes::llm::GlossarySanitizer& glossarySanitizer,
                                       hermes::llm::TranscriptEnhancer& transcriptEnhancer);

        void execute(const Job& job) override;

    private:
        // Si la fuente era un video: deja el audio extraido como nueva fuente
        // de la entrevista y borra el video (no se retiene, ver ADR-016).
        void replaceVideoWithExtractedAudio(int interviewId, const InterviewAudioRecord& video, const std::string& normalizedPath);

        // Turnos con hablante (investigador / sujeto) via diarizacion. Si no
        // hay diarizador disponible o falla, un turno por segmento sin
        // hablante; `diarized` indica cual de los dos casos fue.
        std::vector<hermes::transcript::TranscriptTurn> identifySpeakers(int interviewId,
                                                                         const std::string& normalizedPath,
                                                                         const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                                                         bool& diarized);

        // Corrige en `turns` las variantes de los terminos del glosario
        // (ADR-018) y deja el detalle en glossary_changes.txt. Si Ollama
        // falla, `turns` queda como estaba.
        void applyGlossary(int interviewId,
                           const std::vector<std::string>& keywords,
                           const std::filesystem::path& interviewDir,
                           std::vector<hermes::transcript::TranscriptTurn>& turns);

        IInterviewRepository& m_interviewRepository;
        IInterviewJobRepository& m_jobRepository;
        hermes::audio::IAudioNormalizer& m_audioNormalizer;
        hermes::transcription::ITranscriber& m_transcriber;
        hermes::diarization::IDiarizer& m_diarizer;
        hermes::transcript::ITranscriptStore& m_transcriptStore;
        hermes::llm::GlossarySanitizer& m_glossarySanitizer;
        hermes::llm::TranscriptEnhancer& m_transcriptEnhancer;
};

}  // namespace hermes::jobs
