#include "../include/SherpaOnnxDiarizer.h"
#include "../../api/include/logger.h"
#include "../../audio/include/WavReader.h"
#include "../../third_party/sherpa-onnx/c-api.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <thread>

namespace hermes::diarization {

namespace {

constexpr const wchar_t* C_API_DLL = L"sherpa-onnx-c-api.dll";
constexpr int32_t EXPECTED_SAMPLE_RATE = 16000;

// Parametros validados en el spike sobre entrevistas reales (ADR-021): son
// los defaults de sherpa-onnx. Tramos de menos de 0.3s se descartan (ruido,
// respiraciones); huecos de menos de 0.5s del mismo hablante se unen.
constexpr float MIN_DURATION_ON_S = 0.3f;
constexpr float MIN_DURATION_OFF_S = 0.5f;
// Umbral de distancia del clustering cuando no se fija la cantidad de
// hablantes (el default). Mas alto = menos clusters. Con 0.7 cada persona
// queda en uno o pocos clusters y el ruido en clusters chicos, que
// SpeakerAssigner descarta/reasigna. Forzar 2 clusters fallo en la entrevista
// 7 (49 min): la entrevistada quedo partida en dos clusters y el investigador
// mezclado con una de las mitades. Validado con 0.7 en las entrevistas 6 y 7.
constexpr float CLUSTER_THRESHOLD = 0.7f;

int32_t threadCount() {
    return static_cast<int32_t>(std::max(1u, std::min(4u, std::thread::hardware_concurrency())));
}

template <typename Fn>
Fn loadSymbol(HMODULE module, const char* name) {
    auto* symbol = reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(module, name)));
    if (!symbol) {
        throw std::runtime_error(std::string("sherpa-onnx-c-api.dll no exporta ") + name +
                                 " (version distinta a la esperada, ver ADR-021)");
    }
    return symbol;
}

struct ProgressState {
    int lastLoggedPercent = -1;
};

int32_t logProgress(int32_t done, int32_t total, void* arg) {
    auto* state = static_cast<ProgressState*>(arg);
    if (total <= 0) return 0;
    const int percent = static_cast<int>(100LL * done / total);
    if (percent - state->lastLoggedPercent >= 10 || percent == 100) {
        log_event("[SherpaOnnxDiarizer][diarize] Progreso: " + std::to_string(percent) + "%");
        state->lastLoggedPercent = percent;
    }
    return 0;
}

}  // namespace

struct SherpaOnnxDiarizer::Impl {
    HMODULE module = nullptr;
    decltype(&SherpaOnnxGetVersionStr) getVersion = nullptr;
    decltype(&SherpaOnnxCreateOfflineSpeakerDiarization) create = nullptr;
    decltype(&SherpaOnnxDestroyOfflineSpeakerDiarization) destroy = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationGetSampleRate) getSampleRate = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationProcessWithCallback) process = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments) numSegments = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime) sortByStartTime = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationDestroySegment) destroySegments = nullptr;
    decltype(&SherpaOnnxOfflineSpeakerDiarizationDestroyResult) destroyResult = nullptr;
    const SherpaOnnxOfflineSpeakerDiarization* diarizer = nullptr;

    ~Impl() {
        if (diarizer && destroy) destroy(diarizer);
        if (module) FreeLibrary(module);
    }
};

SherpaOnnxDiarizer::SherpaOnnxDiarizer(SherpaOnnxOptions options) : m_options(std::move(options)) {}

SherpaOnnxDiarizer::~SherpaOnnxDiarizer() = default;

bool SherpaOnnxDiarizer::isAvailable() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_impl) return true;

    const std::filesystem::path dll = std::filesystem::path(m_options.libraryDir) / C_API_DLL;
    std::string missing;
    if (!std::filesystem::exists(dll)) missing = dll.string();
    else if (!std::filesystem::exists(m_options.segmentationModelPath)) missing = m_options.segmentationModelPath;
    else if (!std::filesystem::exists(m_options.embeddingModelPath)) missing = m_options.embeddingModelPath;

    if (!missing.empty()) {
        if (!m_availabilityLogged) {
            log_event("[SherpaOnnxDiarizer][isAvailable] Diarizacion deshabilitada: falta '" + missing +
                      "'. La transcripcion sigue sin identificar hablantes. Ver README (seccion Diarizacion) "
                      "para descargar sherpa-onnx y los modelos; rutas configurables con DIARIZATION_LIB_DIR, "
                      "DIARIZATION_SEGMENTATION_MODEL y DIARIZATION_EMBEDDING_MODEL.");
            m_availabilityLogged = true;
        }
        return false;
    }
    return true;
}

void SherpaOnnxDiarizer::ensureLoaded() {
    if (m_impl) return;

    auto impl = std::make_unique<Impl>();
    // LOAD_WITH_ALTERED_SEARCH_PATH (requiere ruta absoluta): las dependencias
    // de la DLL (onnxruntime.dll) se buscan en su misma carpeta y no en la
    // del ejecutable ni en el PATH del sistema.
    const std::filesystem::path dll = std::filesystem::absolute(std::filesystem::path(m_options.libraryDir) / C_API_DLL);
    impl->module = LoadLibraryExW(dll.wstring().c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!impl->module) {
        throw std::runtime_error("No se pudo cargar " + dll.string() + " (error de Windows " +
                                 std::to_string(GetLastError()) + ")");
    }

    impl->getVersion = loadSymbol<decltype(impl->getVersion)>(impl->module, "SherpaOnnxGetVersionStr");
    impl->create = loadSymbol<decltype(impl->create)>(impl->module, "SherpaOnnxCreateOfflineSpeakerDiarization");
    impl->destroy = loadSymbol<decltype(impl->destroy)>(impl->module, "SherpaOnnxDestroyOfflineSpeakerDiarization");
    impl->getSampleRate = loadSymbol<decltype(impl->getSampleRate)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationGetSampleRate");
    impl->process = loadSymbol<decltype(impl->process)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationProcessWithCallback");
    impl->numSegments = loadSymbol<decltype(impl->numSegments)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationResultGetNumSegments");
    impl->sortByStartTime = loadSymbol<decltype(impl->sortByStartTime)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationResultSortByStartTime");
    impl->destroySegments = loadSymbol<decltype(impl->destroySegments)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationDestroySegment");
    impl->destroyResult = loadSymbol<decltype(impl->destroyResult)>(impl->module, "SherpaOnnxOfflineSpeakerDiarizationDestroyResult");

    log_event(std::string("[SherpaOnnxDiarizer][ensureLoaded] sherpa-onnx ") + impl->getVersion() + " cargado desde " + dll.string());

    SherpaOnnxOfflineSpeakerDiarizationConfig config;
    std::memset(&config, 0, sizeof(config));
    config.segmentation.pyannote.model = m_options.segmentationModelPath.c_str();
    config.segmentation.num_threads = threadCount();
    config.segmentation.provider = "cpu";
    config.embedding.model = m_options.embeddingModelPath.c_str();
    config.embedding.num_threads = threadCount();
    config.embedding.provider = "cpu";
    config.clustering.num_clusters = m_options.numSpeakers > 0 ? m_options.numSpeakers : -1;
    config.clustering.threshold = CLUSTER_THRESHOLD;
    config.min_duration_on = MIN_DURATION_ON_S;
    config.min_duration_off = MIN_DURATION_OFF_S;

    impl->diarizer = impl->create(&config);
    if (!impl->diarizer) {
        throw std::runtime_error("sherpa-onnx no pudo crear el diarizador (revisar los modelos: " +
                                 m_options.segmentationModelPath + ", " + m_options.embeddingModelPath + ")");
    }
    const int32_t sampleRate = impl->getSampleRate(impl->diarizer);
    if (sampleRate != EXPECTED_SAMPLE_RATE) {
        throw std::runtime_error("El diarizador espera audio a " + std::to_string(sampleRate) +
                                 " Hz y el pipeline normaliza a 16000 Hz");
    }
    m_impl = std::move(impl);
}

std::vector<SpeakerTurn> SherpaOnnxDiarizer::diarize(const std::string& audioPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureLoaded();

    const std::vector<float> samples16k = hermes::audio::readNormalizedWav(audioPath);
    if (samples16k.empty()) {
        throw std::runtime_error("El audio normalizado esta vacio: " + audioPath);
    }

    ProgressState progress;
    const SherpaOnnxOfflineSpeakerDiarizationResult* result =
        m_impl->process(m_impl->diarizer, samples16k.data(), static_cast<int32_t>(samples16k.size()), logProgress, &progress);
    if (!result) {
        throw std::runtime_error("sherpa-onnx fallo al diarizar el audio");
    }

    std::vector<SpeakerTurn> turns;
    const int32_t count = m_impl->numSegments(result);
    const SherpaOnnxOfflineSpeakerDiarizationSegment* segments = m_impl->sortByStartTime(result);
    if (segments) {
        turns.reserve(count);
        for (int32_t i = 0; i < count; ++i) {
            turns.push_back({static_cast<int64_t>(segments[i].start * 1000.0f),
                             static_cast<int64_t>(segments[i].end * 1000.0f),
                             segments[i].speaker});
        }
        m_impl->destroySegments(segments);
    }
    m_impl->destroyResult(result);
    return turns;
}

}  // namespace hermes::diarization
