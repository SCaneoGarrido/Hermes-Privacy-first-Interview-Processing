#pragma once

#include "IJobHandler.h"
#include "IInterviewJobRepository.h"
#include "../../api/include/repositories/IInterviewRepository.h"
#include "../../audio/include/IAudioNormalizer.h"
#include "../../transcription/include/ITranscriber.h"

namespace hermes::jobs {

// Orquesta la Fase 1 del pipeline (Sprint 5, ver whisper.cpp Architecture
// en la vault): normaliza el audio subido, lo transcribe, y guarda tanto
// el JSON crudo (interview_jobs.raw_transcript_path, insumo de Sprint 6)
// como un TXT plano sin diarizar (interview_results) para que la
// entrevista tenga un resultado utilizable ya en este sprint, sin esperar
// a que Ollama exista.
//
// No toca DatabaseManager ni whisper.cpp/FFmpeg directamente: todo pasa
// por las interfaces inyectadas (Filosofia de Repositorios).
class InterviewProcessingJobHandler : public IJobHandler {
    public:
        InterviewProcessingJobHandler(IInterviewRepository& interviewRepository,
                                       IInterviewJobRepository& jobRepository,
                                       hermes::audio::IAudioNormalizer& audioNormalizer,
                                       hermes::transcription::ITranscriber& transcriber);

        void execute(const Job& job) override;

    private:
        IInterviewRepository& m_interviewRepository;
        IInterviewJobRepository& m_jobRepository;
        hermes::audio::IAudioNormalizer& m_audioNormalizer;
        hermes::transcription::ITranscriber& m_transcriber;
};

}  // namespace hermes::jobs
