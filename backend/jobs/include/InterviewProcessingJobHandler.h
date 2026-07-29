#pragma once

#include "IJobHandler.h"
#include "IInterviewJobRepository.h"
#include "../../api/include/repositories/IInterviewRepository.h"
#include "../../audio/include/IAudioNormalizer.h"
#include "../../transcription/include/ITranscriber.h"
#include "../../llm/include/TranscriptEnhancer.h"

namespace hermes::jobs {

// Orquesta el pipeline completo de procesamiento (Sprint 5 + Sprint 6, ver
// whisper.cpp Architecture y Ollama Integration Strategy en la vault):
// normaliza el audio, transcribe con whisper.cpp, guarda el JSON crudo
// (interview_jobs.raw_transcript_path) y un TXT plano sin diarizar como
// resultado utilizable de base (Sprint 5). Despues intenta mejorarlo con
// Ollama (correccion + estructuracion por hablante, anonimizacion,
// resumen) - si Ollama falla o no esta disponible, el resultado de Sprint
// 5 sigue siendo el disponible: la ausencia de la mejora no tumba el job.
//
// No toca DatabaseManager, whisper.cpp/FFmpeg, ni Ollama directamente:
// todo pasa por las interfaces inyectadas (Filosofia de Repositorios).
class InterviewProcessingJobHandler : public IJobHandler {
    public:
        InterviewProcessingJobHandler(IInterviewRepository& interviewRepository,
                                       IInterviewJobRepository& jobRepository,
                                       hermes::audio::IAudioNormalizer& audioNormalizer,
                                       hermes::transcription::ITranscriber& transcriber,
                                       hermes::llm::TranscriptEnhancer& transcriptEnhancer);

        void execute(const Job& job) override;

    private:
        IInterviewRepository& m_interviewRepository;
        IInterviewJobRepository& m_jobRepository;
        hermes::audio::IAudioNormalizer& m_audioNormalizer;
        hermes::transcription::ITranscriber& m_transcriber;
        hermes::llm::TranscriptEnhancer& m_transcriptEnhancer;
};

}  // namespace hermes::jobs
