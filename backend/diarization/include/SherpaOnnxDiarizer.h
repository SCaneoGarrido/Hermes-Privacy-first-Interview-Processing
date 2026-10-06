#pragma once

#include "IDiarizer.h"

#include <memory>
#include <mutex>
#include <string>

namespace hermes::diarization {

struct SherpaOnnxOptions {
    // Carpeta con sherpa-onnx-c-api.dll + onnxruntime.dll (release oficial
    // win-x64-shared, version fijada en ADR-021).
    std::string libraryDir;
    std::string segmentationModelPath;  // pyannote segmentation-3.0 (onnx)
    std::string embeddingModelPath;     // embeddings de hablante (onnx)
    // Hablantes a forzar en el clustering. <= 0 (default): clustering por
    // umbral de distancia; SpeakerAssigner reduce los clusters resultantes a
    // investigador + sujeto. Forzar 2 parece natural para una entrevista, pero
    // en audio largo une mal a las personas (ver CLUSTER_THRESHOLD).
    int numSpeakers = 0;
};

// Diarizacion con sherpa-onnx (pyannote segmentation + embeddings + clustering),
// 100% local en CPU. La DLL se carga en runtime (LoadLibrary/GetProcAddress)
// en vez de linkearla: si falta, el backend arranca igual y solo se pierde
// la diarizacion; ademas evita mezclar en el linkeo el toolchain MSVC de la
// release oficial con este build MinGW. Por la API C no cruzan tipos C++:
// toda memoria que devuelve la DLL se libera con sus propias funciones
// Destroy (CRTs distintas no comparten heap).
//
// Carga perezosa y bajo mutex, mismo patron que WhisperTranscriber.
class SherpaOnnxDiarizer : public IDiarizer {
    public:
        explicit SherpaOnnxDiarizer(SherpaOnnxOptions options);
        ~SherpaOnnxDiarizer() override;

        SherpaOnnxDiarizer(const SherpaOnnxDiarizer&) = delete;
        SherpaOnnxDiarizer& operator=(const SherpaOnnxDiarizer&) = delete;

        bool isAvailable() override;
        std::vector<SpeakerTurn> diarize(const std::string& audioPath) override;

    private:
        struct Impl;

        // Debe llamarse con m_mutex tomado.
        void ensureLoaded();

        SherpaOnnxOptions m_options;
        std::mutex m_mutex;
        std::unique_ptr<Impl> m_impl;
        bool m_availabilityLogged = false;
};

}  // namespace hermes::diarization
