#include "../include/WhisperTranscriber.h"
#include "../../api/include/logger.h"
#include "../../audio/include/WavReader.h"

#include <ggml-backend.h>
#include <whisper.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace hermes::transcription {

namespace {

constexpr int SAMPLE_RATE = 16000;
constexpr int64_t SAMPLES_PER_CENTISECOND = SAMPLE_RATE / 100;

// Tokens de texto previo (de la ventana anterior) con los que se condiciona
// cada ventana de 30s en la pasada principal: ninguno. El texto previo
// arrastra derivas: con el default (224) un bucle se autoalimento hasta el
// final del audio (entrevista 13, x114); con 64 o 32, una ventana sin puntuar
// dejo sin puntuacion el resto de la entrevista (6 y 2). Cada ventana se
// condiciona solo con el prompt fijo (glosario + STYLE_PROMPT).
constexpr int MAIN_PASS_MAX_TEXT_CTX = 0;

// Ejemplo de habla puntuada que se repite como contexto en cada ventana:
// whisper imita la forma del texto previo. Es dialogo neutro a proposito: una
// version anterior ("Transcripción fiel de una entrevista, con puntuación...")
// era metatexto, whisper la copiaba en tramos dificiles y la palabra
// "Transcripción" disparaba creditos de subtitulos alucinados (ADR-020). Si
// igual la copia, isPromptEcho la descarta y repairGaps recupera el tramo.
constexpr std::string_view STYLE_PROMPT = "¿Y cómo lo hacen ustedes? Bueno, depende del caso, pero en general sí.";

// Bucle de alucinacion: segmentos consecutivos que repiten lo mismo. Un texto
// largo (>= LOOP_CONTAINMENT_MIN_CHARS normalizado) cuenta como repeticion si
// contiene al otro o esta contenido en el (whisper repite con variaciones: la
// misma frase duplicada, un "Sí," delante); uno corto tiene que ser identico,
// porque "Sí." / "Sí, claro." seguidos son habla normal.
constexpr size_t LOOP_MIN_REPEATS_LONG = 3;
constexpr size_t LOOP_MIN_REPEATS_SHORT = 4;
constexpr size_t LOOP_CONTAINMENT_MIN_CHARS = 15;

// Tramo sin ningun signo de puntuacion a partir del cual se considera que
// whisper derivo a texto sin puntuar y se re-transcribe (repairUnpunctuated).
constexpr int64_t UNPUNCTUATED_MIN_CS = 6000;  // 60s

// Hueco entre segmentos a partir del cual se re-transcribe (repairGaps):
// whisper descarta ventanas enteras de 20-30s cuando la decodificacion no lo
// convence (no_speech_prob > 0.6 y avg_logprob < -1), aunque haya habla.
constexpr int64_t GAP_MIN_CS = 800;  // 8s
// En la re-transcripcion de huecos, whisper descarta menos: el VAD ya dice si
// hay habla, y si el hueco era silencio no devuelve nada.
constexpr float GAP_NO_SPEECH_THOLD = 0.8f;
constexpr float DEFAULT_NO_SPEECH_THOLD = 0.6f;  // default de whisper.cpp 1.8.6

// Eco del prompt (whisper copia su contexto cuando no entiende el audio): el
// segmento contiene un tramo continuo del prompt de al menos
// ECHO_MIN_COPY_CHARS (sin espacios ni puntuacion) que ocupa al menos
// ECHO_MIN_SHARE del segmento. Se compara texto continuo y no palabras
// sueltas porque la frase de estilo usa palabras comunes ("bueno", "pero").
constexpr size_t ECHO_MIN_COPY_CHARS = 25;
constexpr double ECHO_MIN_SHARE = 0.5;

// Beam search: mas robusto que greedy (menos palabras inventadas/omitidas),
// pero varias veces mas lento. En GPU se usa en la pasada principal; en CPU
// solo para re-transcribir tramos en bucle (unos minutos, no la entrevista).
constexpr int BEAM_SIZE = 5;

// Tope de tokens del glosario como initial_prompt. whisper.cpp acota todo el
// contexto de texto a n_text_ctx/2 = 224 tokens (whisper.cpp:6913); con 150
// para el glosario quedan los de MAIN_PASS_MAX_TEXT_CTX + el marcador.
constexpr int MAX_PROMPT_TOKENS = 150;
constexpr int WHISPER_MAX_TEXT_CTX = 224;

// Parametros del VAD (Silero). Los defaults de whisper.cpp recortan de mas
// para entrevistas: 30ms de padding come el inicio/fin de las palabras y
// 100ms de silencio parte frases en pausas normales de quien piensa lo que
// dice. Valores de partida a validar con el A/B (ver ADR-020).
constexpr float VAD_THRESHOLD = 0.45f;
constexpr int VAD_MIN_SPEECH_MS = 250;
constexpr int VAD_MIN_SILENCE_MS = 400;
constexpr int VAD_SPEECH_PAD_MS = 200;
constexpr float VAD_MAX_SPEECH_S = 30.0f;

// Modelos por encima de este tamaño (medium, large-v3) en CPU son varias
// veces mas lentos que small: se avisa en el log para que no parezca colgado.
constexpr std::uintmax_t LARGE_MODEL_BYTES = 1'000'000'000;

// Frases que whisper inventa en silencios o ruido porque aparecen al final
// de muchos videos subtitulados con los que fue entrenado. Solo se descarta
// un segmento si su texto COMPLETO (normalizado) es una de estas y dura
// poco: una persona real puede decir "gracias" en una entrevista.
constexpr int64_t HALLUCINATION_MAX_CS = 300;
constexpr std::array<std::string_view, 5> HALLUCINATION_PHRASES = {
    "graciasporverelvideo",
    "graciasporver",
    "suscribetealcanal",
    "noolvidessuscribirte",
    "subtitulosrealizadosporlacomunidaddeamaraorg",
};

// Densidad maxima creible de texto por segmento. El habla real en espanol
// ronda 12-18 caracteres por segundo; whisper (large-v3, sobre todo) a veces
// emite segmentos de ~0.1s con una frase entera repetida de lo dicho justo
// antes (entrevista 6, 01:49: tres copias de la pregunta en 100-360ms). El
// margen fijo protege monosilabos ("Sí.") con tiempos muy ajustados. Validado
// sobre los 379 segmentos de esa entrevista: marca solo las 3 alucinaciones.
constexpr double MAX_CHARS_PER_SECOND = 35.0;
constexpr size_t DENSITY_MARGIN_CHARS = 10;

size_t utf8Length(const std::string& text) {
    size_t count = 0;
    for (unsigned char c : text) {
        if ((c & 0xC0) != 0x80) ++count;  // no cuenta bytes de continuacion
    }
    return count;
}

bool isImplausiblyDense(const RawSegment& segment) {
    const double seconds = static_cast<double>(std::max<int64_t>(segment.t1 - segment.t0, 0)) / 100.0;
    return static_cast<double>(utf8Length(segment.text)) > seconds * MAX_CHARS_PER_SECOND + DENSITY_MARGIN_CHARS;
}

// Texto comparable: minusculas, sin puntuacion ni espacios ("Que no se
// cumplen los plazos," == "que no se cumplen los plazos"). Los bytes no
// ASCII (acentos en UTF-8) se conservan tal cual.
std::string normalizeForComparison(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char c : text) {
        if (c >= 0x80 || std::isalnum(c)) {
            normalized += static_cast<char>(std::tolower(c));
        }
    }
    return normalized;
}

// Saca tildes/dieresis de las vocales y la ñ (UTF-8, minusculas) para
// comparar sin depender de como whisper acentuo: "subtítulos" == "subtitulos".
std::string foldAccents(std::string text) {
    static const std::array<std::pair<std::string_view, char>, 7> MAP = {{
        {"\xC3\xA1", 'a'}, {"\xC3\xA9", 'e'}, {"\xC3\xAD", 'i'}, {"\xC3\xB3", 'o'},
        {"\xC3\xBA", 'u'}, {"\xC3\xBC", 'u'}, {"\xC3\xB1", 'n'},
    }};
    for (const auto& [from, to] : MAP) {
        size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos) {
            text.replace(pos, from.size(), 1, to);
            ++pos;
        }
    }
    // Signos de apertura "¿" y "¡": puntuacion, whisper a veces los omite.
    for (const std::string_view mark : {std::string_view("\xC2\xBF"), std::string_view("\xC2\xA1")}) {
        size_t pos = 0;
        while ((pos = text.find(mark, pos)) != std::string::npos) text.erase(pos, mark.size());
    }
    return text;
}

// Creditos de subtitulado que whisper aprendio de su entrenamiento y emite en
// audio que no entiende ("Transcripción y subtítulos por José Miguel Pinto...",
// entrevista 1). Inequivocos en una entrevista: se descartan donde aparezcan,
// sin limite de duracion. Comparados sin espacios ni tildes.
constexpr std::array<std::string_view, 4> HALLUCINATION_SUBSTRINGS = {
    "amaraorg",
    "subtitulospor",
    "subtituladopor",
    "transcripcionysubtitulos",
};

bool isKnownHallucination(const RawSegment& segment) {
    const std::string normalized = foldAccents(normalizeForComparison(segment.text));
    if (normalized.empty()) return false;
    for (const auto pattern : HALLUCINATION_SUBSTRINGS) {
        if (normalized.find(pattern) != std::string::npos) return true;
    }
    if (segment.t1 - segment.t0 > HALLUCINATION_MAX_CS) return false;
    return std::find(HALLUCINATION_PHRASES.begin(), HALLUCINATION_PHRASES.end(), normalized) != HALLUCINATION_PHRASES.end();
}

// Palabras normalizadas (minusculas, sin puntuacion ni tildes).
std::vector<std::string> normalizedWords(const std::string& text) {
    std::vector<std::string> words;
    std::string current;
    for (unsigned char c : text) {
        if (c >= 0x80 || std::isalnum(c)) {
            current += static_cast<char>(std::tolower(c));
        } else if (!current.empty()) {
            words.push_back(foldAccents(current));
            current.clear();
        }
    }
    if (!current.empty()) words.push_back(foldAccents(current));
    return words;
}

// Largo del tramo continuo mas largo comun a a y b (programacion dinamica
// con dos filas: segmento ~cientos de bytes x prompt <1000, barato).
size_t longestCommonSubstring(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return 0;
    std::vector<size_t> previous(b.size() + 1, 0);
    std::vector<size_t> current(b.size() + 1, 0);
    size_t best = 0;
    for (size_t i = 1; i <= a.size(); ++i) {
        for (size_t j = 1; j <= b.size(); ++j) {
            current[j] = a[i - 1] == b[j - 1] ? previous[j - 1] + 1 : 0;
            best = std::max(best, current[j]);
        }
        std::swap(previous, current);
    }
    return best;
}

// Segmento que repite el prompt (glosario o frase de estilo) en vez de
// transcribir el audio. promptNormalized: prompt sin espacios, puntuacion ni
// tildes (ver normalizeForComparison / foldAccents).
bool isPromptEcho(const RawSegment& segment, const std::string& promptNormalized) {
    if (promptNormalized.empty()) return false;
    const std::string text = foldAccents(normalizeForComparison(segment.text));
    if (text.size() < ECHO_MIN_COPY_CHARS) return false;
    const size_t copied = longestCommonSubstring(text, promptNormalized);
    return copied >= ECHO_MIN_COPY_CHARS && static_cast<double>(copied) >= ECHO_MIN_SHARE * static_cast<double>(text.size());
}

bool hasPunctuation(const std::string& text) {
    return text.find_first_of(".,?!") != std::string::npos || text.find("\xC2\xBF") != std::string::npos;  // "¿"
}

size_t wordCount(const std::vector<RawSegment>& segments) {
    size_t count = 0;
    for (const auto& segment : segments) count += normalizedWords(segment.text).size();
    return count;
}

// Mismo enunciado repetido: identico, o (si es largo) uno contiene al otro.
bool sameUtterance(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return false;
    if (a == b) return true;
    if (std::min(a.size(), b.size()) < LOOP_CONTAINMENT_MIN_CHARS) return false;
    return a.find(b) != std::string::npos || b.find(a) != std::string::npos;
}

struct RepeatRun {
    size_t first;  // indice del primer segmento del bucle (inclusive)
    size_t last;   // indice del ultimo segmento del bucle (inclusive)
};

std::vector<RepeatRun> findRepeatRuns(const std::vector<RawSegment>& segments) {
    std::vector<RepeatRun> runs;
    if (segments.empty()) return runs;

    size_t start = 0;
    std::string startText = normalizeForComparison(segments[0].text);
    for (size_t i = 1; i <= segments.size(); ++i) {
        const std::string text = i < segments.size() ? normalizeForComparison(segments[i].text) : std::string();
        if (i < segments.size() && sameUtterance(text, startText)) continue;
        const size_t minRepeats = startText.size() >= LOOP_CONTAINMENT_MIN_CHARS ? LOOP_MIN_REPEATS_LONG : LOOP_MIN_REPEATS_SHORT;
        if (i - start >= minRepeats) {
            runs.push_back({start, i - 1});
        }
        start = i;
        startText = text;
    }
    return runs;
}

std::string formatClock(int64_t centiseconds) {
    const int64_t totalSeconds = centiseconds / 100;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld",
                  static_cast<long long>(totalSeconds / 60), static_cast<long long>(totalSeconds % 60));
    return std::string(buf);
}

// Quita los segmentos que no son transcripcion del audio: creditos de
// subtitulado y frases conocidas, densidad de texto imposible, y eco del
// glosario. Cada descarte queda en el log para poder auditarlo.
// Un segmento con densidad imposible solo es alucinacion si ademas repite lo
// dicho cerca (entrevista 6, 01:49: tres copias de la pregunta anterior en
// 100-360ms). Si su texto es unico, es habla real con la marca de tiempo rota
// (entrevista 2, 44:42-45:16: diez segmentos de 100ms con la pregunta real) y
// se conserva.
constexpr size_t DUPLICATE_NEIGHBOR_WINDOW = 3;

bool repeatsNeighbor(const std::vector<RawSegment>& segments, size_t index) {
    const std::string text = normalizeForComparison(segments[index].text);
    if (text.empty()) return false;
    const size_t from = index > DUPLICATE_NEIGHBOR_WINDOW ? index - DUPLICATE_NEIGHBOR_WINDOW : 0;
    const size_t to = std::min(segments.size(), index + DUPLICATE_NEIGHBOR_WINDOW + 1);
    for (size_t j = from; j < to; ++j) {
        if (j == index) continue;
        const std::string other = normalizeForComparison(segments[j].text);
        if (!other.empty() && (other.find(text) != std::string::npos || text.find(other) != std::string::npos)) return true;
    }
    return false;
}

std::vector<RawSegment> dropHallucinations(std::vector<RawSegment> segments, const std::string& promptNormalized) {
    std::vector<RawSegment> kept;
    kept.reserve(segments.size());
    for (size_t i = 0; i < segments.size(); ++i) {
        auto& segment = segments[i];
        std::string reason;
        if (isKnownHallucination(segment)) {
            reason = "alucinacion conocida";
        } else if (isImplausiblyDense(segment) && repeatsNeighbor(segments, i)) {
            reason = "densidad imposible repitiendo un segmento cercano (" + std::to_string(utf8Length(segment.text)) +
                     " caracteres en " + std::to_string((segment.t1 - segment.t0) * 10) + "ms)";
        } else if (isPromptEcho(segment, promptNormalized)) {
            reason = "eco del glosario";
        }
        if (!reason.empty()) {
            log_event("[WhisperTranscriber][transcribe] Segmento descartado por " + reason + " (" + formatClock(segment.t0) +
                      "): " + segment.text);
            continue;
        }
        kept.push_back(std::move(segment));
    }
    return kept;
}

// Sin esto, un audio largo no deja ningun rastro en el log entre
// "Transcribiendo con whisper.cpp" y el resultado final - parece trabado
// aunque este funcionando. Se loguea cada 10% en vez de en cada llamada
// (whisper.cpp invoca este callback muy seguido).
struct ProgressLogState {
    int lastLoggedPercent = -1;
};

void logWhisperProgress(struct whisper_context* /*ctx*/, struct whisper_state* /*state*/, int progress, void* user_data) {
    auto* logState = static_cast<ProgressLogState*>(user_data);
    if (progress - logState->lastLoggedPercent >= 10 || progress == 100) {
        log_event("[WhisperTranscriber][transcribe] Progreso: " + std::to_string(progress) + "%");
        logState->lastLoggedPercent = progress;
    }
}

// whisper.cpp decodifica por token BPE, no por caracter: en audio largo/
// ruidoso puede emitir un token cuyos bytes truncan una secuencia UTF-8
// multibyte a mitad de camino. Eso produce texto con bytes invalidos que
// nlohmann::json::dump() rechaza con type_error.316 - visto en produccion
// con un audio real de mas de 100MB. Se repara reemplazando cualquier
// secuencia invalida por U+FFFD, en vez de dejar que el error tumbe el job.
std::string sanitizeUtf8(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    size_t i = 0;

    while (i < input.size()) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        int len = 0;
        if ((c & 0x80) == 0x00) len = 1;        // 0xxxxxxx
        else if ((c & 0xE0) == 0xC0) len = 2;   // 110xxxxx
        else if ((c & 0xF0) == 0xE0) len = 3;   // 1110xxxx
        else if ((c & 0xF8) == 0xF0) len = 4;   // 11110xxx

        if (len == 0 || i + len > input.size()) {
            output += "\xEF\xBF\xBD";  // U+FFFD
            ++i;
            continue;
        }

        bool validContinuation = true;
        for (int k = 1; k < len; ++k) {
            const unsigned char cc = static_cast<unsigned char>(input[i + k]);
            if ((cc & 0xC0) != 0x80) {
                validContinuation = false;
                break;
            }
        }

        if (validContinuation) {
            output.append(input, i, len);
        } else {
            output += "\xEF\xBF\xBD";
        }
        i += len;
    }

    return output;
}

std::string trimSpaces(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Dispositivo GPU que ggml ve en este build (Vulkan), o nullptr si no hay
// ninguno (build sin backend GPU, o maquina sin driver Vulkan).
ggml_backend_dev_t findGpuDevice() {
    return ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
}

}  // namespace

WhisperTranscriber::WhisperTranscriber(WhisperOptions options) : m_options(std::move(options)) {}

WhisperTranscriber::~WhisperTranscriber() {
    if (m_context) {
        whisper_free(m_context);
    }
}

void WhisperTranscriber::ensureModelLoaded() {
    if (m_context) {
        return;
    }

    if (!std::filesystem::exists(m_options.modelPath)) {
        throw std::runtime_error(
            "Modelo de whisper.cpp no encontrado en '" + m_options.modelPath +
            "'. Descargalo con models/download-ggml-model.sh (whisper.cpp) o desde "
            "https://huggingface.co/ggerganov/whisper.cpp y coloca el .bin en esa ruta "
            "(configurable con la env var WHISPER_MODEL_PATH). Ver 'whisper.cpp Architecture' en la vault.");
    }

    ggml_backend_dev_t gpuDevice = nullptr;
    if (m_options.gpuMode != GpuMode::Off) {
        gpuDevice = findGpuDevice();
        if (!gpuDevice) {
            log_event("[WhisperTranscriber][ensureModelLoaded] No se detecto GPU compatible (Vulkan), se usa CPU");
        }
    }

    auto load = [this](bool useGpu) {
        whisper_context_params params = whisper_context_default_params();
        params.use_gpu = useGpu;
        // El default de whisper.cpp 1.8.6 trae flash_attn=true. La ruta CPU
        // de flash attention crasheo en este build (MinGW estatico, acceso a
        // memoria invalido sin excepcion capturable): solo se activa en GPU.
        params.flash_attn = useGpu;
        return whisper_init_from_file_with_params(m_options.modelPath.c_str(), params);
    };

    if (gpuDevice) {
        m_context = load(true);
        if (m_context) {
            m_gpuActive = true;
            log_event(std::string("[WhisperTranscriber][ensureModelLoaded] Modelo cargado en GPU: ") +
                      ggml_backend_dev_description(gpuDevice) + " (" + m_options.modelPath + ")");
            return;
        }
        log_event("[WhisperTranscriber][ensureModelLoaded] No se pudo cargar el modelo en GPU, se reintenta en CPU");
    }

    m_context = load(false);
    if (!m_context) {
        throw std::runtime_error("whisper.cpp no pudo cargar el modelo: " + m_options.modelPath);
    }
    m_gpuActive = false;
    log_event("[WhisperTranscriber][ensureModelLoaded] Modelo cargado en CPU (" + m_options.modelPath + ")");

    std::error_code ec;
    if (std::filesystem::file_size(m_options.modelPath, ec) > LARGE_MODEL_BYTES && !ec) {
        log_event("[WhisperTranscriber][ensureModelLoaded] Modelo grande en CPU: la transcripcion sera varias veces mas "
                  "lenta. Para CPU conviene WHISPER_MODEL_PATH=./models/ggml-small.bin");
    }
}

std::string WhisperTranscriber::buildInitialPrompt(const std::vector<std::string>& keywords, int& promptTokens) {
    std::vector<whisper_token> tokens(MAX_PROMPT_TOKENS * 2);
    auto countTokens = [&](const std::string& text) {
        return whisper_tokenize(m_context, text.c_str(), tokens.data(), static_cast<int>(tokens.size()));
    };

    // "Glosario: a, b, c. <estilo>": la frase de estilo va al final (lo mas
    // cercano al audio, lo que mas pesa) y los terminos entran mientras quepan.
    const std::string style(STYLE_PROMPT);
    std::string prompt = style;
    promptTokens = countTokens(prompt);
    std::string glossary;
    size_t used = 0;
    for (const auto& keyword : keywords) {
        const std::string candidateGlossary = (glossary.empty() ? "Glosario: " : glossary.substr(0, glossary.size() - 1) + ", ") + keyword + ".";
        const std::string candidate = candidateGlossary + " " + style;
        const int n = countTokens(candidate);
        if (n < 0 || n > MAX_PROMPT_TOKENS) break;
        glossary = candidateGlossary;
        prompt = candidate;
        promptTokens = n;
        ++used;
    }
    if (used < keywords.size()) {
        log_event("[WhisperTranscriber][transcribe] Glosario demasiado largo para whisper: se usan " + std::to_string(used) +
                  " de " + std::to_string(keywords.size()) + " terminos (la sanitizacion posterior usa todos)");
    }
    return prompt;
}

std::vector<TranscriptSegment> WhisperTranscriber::transcribe(const std::string& audioPath,
                                                              const TranscriptionOptions& options) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ensureModelLoaded();

    const std::vector<float> samples = hermes::audio::readNormalizedWav(audioPath);
    if (samples.empty()) {
        throw std::runtime_error("El audio normalizado esta vacio: " + audioPath);
    }

    const bool useVad = std::filesystem::exists(m_options.vadModelPath);
    if (!useVad) {
        log_event("[WhisperTranscriber][transcribe] Modelo VAD no encontrado en '" + m_options.vadModelPath +
                  "', se transcribe sin VAD (mayor riesgo de repeticiones alucinadas en silencios). "
                  "Configurable con WHISPER_VAD_MODEL_PATH.");
    }

    // Glosario (ADR-018): carry_initial_prompt lo repite en cada ventana de
    // 30s, no solo en la primera. n_max_text_ctx es el presupuesto TOTAL de
    // contexto (glosario + texto previo), asi que se le suma el glosario para
    // no comerse el contexto dinamico.
    int promptTokens = 0;
    const std::string initialPrompt = buildInitialPrompt(options.keywords, promptTokens);
    const std::string promptNormalized = foldAccents(normalizeForComparison(initialPrompt));
    auto applyPrompt = [&](whisper_full_params& p, int dynamicContext) {
        if (promptTokens <= 0) {
            p.n_max_text_ctx = dynamicContext;
            return;
        }
        p.initial_prompt = initialPrompt.c_str();
        p.carry_initial_prompt = true;
        p.n_max_text_ctx = std::min(WHISPER_MAX_TEXT_CTX, promptTokens + 1 + dynamicContext);
    };

    // Pasada principal: beam search en GPU (fidelidad), greedy en CPU
    // (velocidad). Contexto previo acotado en ambos casos.
    whisper_full_params params = whisper_full_default_params(m_gpuActive ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);
    if (m_gpuActive) {
        params.beam_search.beam_size = BEAM_SIZE;
    }
    applyPrompt(params, MAIN_PASS_MAX_TEXT_CTX);
    ProgressLogState progressLogState;
    params.progress_callback = logWhisperProgress;
    params.progress_callback_user_data = &progressLogState;
    log_event(std::string("[WhisperTranscriber][transcribe] Pasada principal: ") +
              (m_gpuActive ? "GPU, beam search " + std::to_string(BEAM_SIZE) : std::string("CPU, greedy")) +
              (useVad ? ", con VAD" : ", sin VAD"));

    std::vector<RawSegment> segments = runWhisper(params, samples.data(), samples.size(), 0, useVad);

    // Reparacion de bucles: se recorre de atras hacia adelante para que
    // reemplazar un tramo no invalide los indices de los tramos anteriores.
    const auto runs = findRepeatRuns(segments);
    for (auto it = runs.rbegin(); it != runs.rend(); ++it) {
        const int64_t t0 = segments[it->first].t0;
        const int64_t t1 = segments[it->last].t1;
        const std::string range = formatClock(t0) + "-" + formatClock(t1);
        log_event("[WhisperTranscriber][transcribe] Bucle detectado (" + std::to_string(it->last - it->first + 1) +
                  " segmentos repetidos, " + range + "), re-transcribiendo el tramo");

        // Sin texto previo (contexto dinamico 0): el bucle se alimentaba
        // justamente de eso. El glosario, si hay, se mantiene.
        whisper_full_params retryParams = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
        retryParams.beam_search.beam_size = BEAM_SIZE;
        applyPrompt(retryParams, 0);

        const size_t firstSample = std::min(samples.size(), static_cast<size_t>(t0 * SAMPLES_PER_CENTISECOND));
        const size_t lastSample = std::min(samples.size(), static_cast<size_t>(t1 * SAMPLES_PER_CENTISECOND));
        std::vector<RawSegment> retry;
        if (lastSample > firstSample) {
            retry = runWhisper(retryParams, samples.data() + firstSample, lastSample - firstSample, t0, useVad);
        }

        std::vector<RawSegment> replacement;
        if (!retry.empty() && findRepeatRuns(retry).empty()) {
            log_event("[WhisperTranscriber][transcribe] Tramo " + range + " recuperado (" + std::to_string(retry.size()) + " segmentos)");
            replacement = std::move(retry);
        } else {
            // Se conserva la primera aparicion (probablemente dicha de verdad)
            // y se marca el resto, para que el investigador sepa que escuchar.
            const RawSegment firstOccurrence = segments[it->first];
            const std::string lost = formatClock(firstOccurrence.t1) + "-" + formatClock(t1);
            log_event("[WhisperTranscriber][transcribe] No se pudo recuperar el tramo, se marca como no transcrito: " + lost);
            replacement.push_back(firstOccurrence);
            replacement.push_back({firstOccurrence.t1, t1, "[audio no transcrito " + lost + "]", {}});
        }

        segments.erase(segments.begin() + it->first, segments.begin() + it->last + 1);
        segments.insert(segments.begin() + it->first, replacement.begin(), replacement.end());
    }

    segments = dropHallucinations(std::move(segments), promptNormalized);
    repairUnpunctuated(samples, useVad, promptNormalized, segments);
    repairGaps(samples, useVad, promptNormalized, segments);

    std::vector<TranscriptSegment> result;
    result.reserve(segments.size());
    for (auto& segment : segments) {
        result.push_back({segment.t0 * 10, segment.t1 * 10, std::move(segment.text), std::move(segment.words)});
    }
    return result;
}

std::vector<RawSegment> WhisperTranscriber::retranscribe(const std::vector<float>& samples,
                                                         int64_t t0Cs,
                                                         int64_t t1Cs,
                                                         bool useVad,
                                                         float noSpeechThold) {
    // Sin prompt ni texto previo: cada ventana se decodifica solo con el
    // audio, sin nada que copiar ni deriva que arrastrar.
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
    params.beam_search.beam_size = BEAM_SIZE;
    params.n_max_text_ctx = 0;
    params.no_speech_thold = noSpeechThold;

    const size_t firstSample = std::min(samples.size(), static_cast<size_t>(std::max<int64_t>(t0Cs, 0) * SAMPLES_PER_CENTISECOND));
    const size_t lastSample = std::min(samples.size(), static_cast<size_t>(std::max<int64_t>(t1Cs, 0) * SAMPLES_PER_CENTISECOND));
    if (lastSample <= firstSample) return {};
    return runWhisper(params, samples.data() + firstSample, lastSample - firstSample, t0Cs, useVad);
}

void WhisperTranscriber::repairUnpunctuated(const std::vector<float>& samples,
                                            bool useVad,
                                            const std::string& promptNormalized,
                                            std::vector<RawSegment>& segments) {
    // Tramos [first, last] de segmentos consecutivos sin ningun signo de
    // puntuacion que duran UNPUNCTUATED_MIN_CS o mas: whisper derivo a texto
    // sin puntuar (lo arrastra el contexto previo hasta el final del audio).
    std::vector<std::pair<size_t, size_t>> runs;
    for (size_t i = 0; i < segments.size();) {
        if (hasPunctuation(segments[i].text)) {
            ++i;
            continue;
        }
        size_t j = i;
        while (j + 1 < segments.size() && !hasPunctuation(segments[j + 1].text)) ++j;
        if (segments[j].t1 - segments[i].t0 >= UNPUNCTUATED_MIN_CS) runs.emplace_back(i, j);
        i = j + 1;
    }

    // De atras hacia adelante: reemplazar un tramo no invalida los anteriores.
    for (auto it = runs.rbegin(); it != runs.rend(); ++it) {
        const auto [first, last] = *it;
        const int64_t t0 = segments[first].t0;
        const int64_t t1 = segments[last].t1;
        const std::string range = formatClock(t0) + "-" + formatClock(t1);
        log_event("[WhisperTranscriber][repairUnpunctuated] Tramo sin puntuacion (" + range + ", " +
                  std::to_string(last - first + 1) + " segmentos), re-transcribiendo sin contexto");

        std::vector<RawSegment> retry = dropHallucinations(retranscribe(samples, t0, t1, useVad, DEFAULT_NO_SPEECH_THOLD), promptNormalized);
        size_t punctuated = 0;
        for (const auto& segment : retry) {
            if (hasPunctuation(segment.text)) ++punctuated;
        }
        const std::vector<RawSegment> original(segments.begin() + static_cast<long>(first), segments.begin() + static_cast<long>(last) + 1);
        const size_t originalWords = wordCount(original);
        const size_t retryWords = wordCount(retry);

        // Se acepta solo si de verdad mejora: puntuado, sin bucles y sin
        // perder contenido (el texto sin puntuar sigue siendo mejor que nada).
        if (retry.empty() || punctuated * 2 < retry.size() || retryWords * 10 < originalWords * 7 || !findRepeatRuns(retry).empty()) {
            log_event("[WhisperTranscriber][repairUnpunctuated] Tramo " + range + " no mejoro (" + std::to_string(punctuated) + "/" +
                      std::to_string(retry.size()) + " segmentos puntuados, " + std::to_string(retryWords) + " vs " +
                      std::to_string(originalWords) + " palabras), se conserva el original");
            continue;
        }
        log_event("[WhisperTranscriber][repairUnpunctuated] Tramo " + range + " recuperado con puntuacion (" +
                  std::to_string(retry.size()) + " segmentos, " + std::to_string(retryWords) + " vs " +
                  std::to_string(originalWords) + " palabras)");
        segments.erase(segments.begin() + static_cast<long>(first), segments.begin() + static_cast<long>(last) + 1);
        segments.insert(segments.begin() + static_cast<long>(first), retry.begin(), retry.end());
    }
}

void WhisperTranscriber::repairGaps(const std::vector<float>& samples,
                                    bool useVad,
                                    const std::string& promptNormalized,
                                    std::vector<RawSegment>& segments) {
    // Huecos sin texto de GAP_MIN_CS o mas, incluido el inicio y el final del
    // audio. Muchos son silencio real (el VAD no devuelve nada ahi); otros son
    // ventanas que whisper descarto con habla adentro.
    std::vector<std::pair<int64_t, int64_t>> gaps;
    int64_t previousEnd = 0;
    for (const auto& segment : segments) {
        if (segment.t0 - previousEnd >= GAP_MIN_CS) gaps.emplace_back(previousEnd, segment.t0);
        previousEnd = std::max(previousEnd, segment.t1);
    }
    const int64_t audioEnd = static_cast<int64_t>(samples.size()) / SAMPLES_PER_CENTISECOND;
    if (audioEnd - previousEnd >= GAP_MIN_CS) gaps.emplace_back(previousEnd, audioEnd);
    if (gaps.empty()) return;

    std::vector<RawSegment> recovered;
    size_t recoveredGaps = 0;
    for (const auto& [t0, t1] : gaps) {
        std::vector<RawSegment> retry = dropHallucinations(retranscribe(samples, t0, t1, useVad, GAP_NO_SPEECH_THOLD), promptNormalized);
        if (retry.empty() || !findRepeatRuns(retry).empty()) continue;
        ++recoveredGaps;
        log_event("[WhisperTranscriber][repairGaps] Hueco " + formatClock(t0) + "-" + formatClock(t1) + " recuperado (" +
                  std::to_string(retry.size()) + " segmentos, " + std::to_string(wordCount(retry)) + " palabras)");
        recovered.insert(recovered.end(), std::make_move_iterator(retry.begin()), std::make_move_iterator(retry.end()));
    }
    log_event("[WhisperTranscriber][repairGaps] Huecos revisados: " + std::to_string(gaps.size()) + ", con habla recuperada: " +
              std::to_string(recoveredGaps));
    if (recovered.empty()) return;

    segments.insert(segments.end(), std::make_move_iterator(recovered.begin()), std::make_move_iterator(recovered.end()));
    std::stable_sort(segments.begin(), segments.end(), [](const RawSegment& a, const RawSegment& b) { return a.t0 < b.t0; });
}

std::vector<RawSegment> WhisperTranscriber::runWhisper(whisper_full_params params,
                                                       const float* samples,
                                                       size_t sampleCount,
                                                       int64_t offsetCs,
                                                       bool useVad) {
    params.language = m_options.language.c_str();
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    // No conviene paralelizar de mas en una laptop de researcher (mismo
    // criterio que WORKER_POOL_SIZE=1 por defecto en Sprint 4). Con GPU los
    // hilos de CPU pesan poco igual.
    params.n_threads = static_cast<int>(std::min(4u, std::thread::hardware_concurrency()));
    // Descarta tokens de no-habla ("[Música]", "♪", "(risas)"): no son
    // contenido de la entrevista y confunden la diarizacion/correccion.
    params.suppress_nst = true;
    // Tiempos por token, para ubicar cada palabra (ver extractWords).
    params.token_timestamps = true;
    // whisper.cpp remapea los timestamps de SEGMENTO al audio original cuando
    // usa VAD (no los de token - ver extractWords).
    params.vad = useVad;
    params.vad_model_path = useVad ? m_options.vadModelPath.c_str() : nullptr;
    if (useVad) {
        params.vad_params = whisper_vad_default_params();
        params.vad_params.threshold = VAD_THRESHOLD;
        params.vad_params.min_speech_duration_ms = VAD_MIN_SPEECH_MS;
        params.vad_params.min_silence_duration_ms = VAD_MIN_SILENCE_MS;
        params.vad_params.speech_pad_ms = VAD_SPEECH_PAD_MS;
        params.vad_params.max_speech_duration_s = VAD_MAX_SPEECH_S;
    }

    if (whisper_full(m_context, params, samples, static_cast<int>(sampleCount)) != 0) {
        throw std::runtime_error("whisper.cpp fallo al transcribir");
    }

    const int segmentCount = whisper_full_n_segments(m_context);
    std::vector<RawSegment> segments;
    segments.reserve(segmentCount);
    for (int i = 0; i < segmentCount; ++i) {
        const int64_t t0 = offsetCs + whisper_full_get_segment_t0(m_context, i);
        const int64_t t1 = offsetCs + whisper_full_get_segment_t1(m_context, i);
        segments.push_back({t0, t1, sanitizeUtf8(whisper_full_get_segment_text(m_context, i)), extractWords(i, t0, t1)});
    }
    return segments;
}

std::vector<TranscriptWord> WhisperTranscriber::extractWords(int segmentIndex, int64_t segT0Cs, int64_t segT1Cs) {
    // Agrupa tokens BPE en palabras: un token que empieza con espacio abre
    // una palabra nueva. Los tokens especiales (timestamps, [_BEG_], etc.)
    // tienen id >= eot y no son texto.
    struct PendingWord {
        std::string text;
        int64_t rawT0;
        int64_t rawT1;
    };
    std::vector<PendingWord> pending;
    const whisper_token eot = whisper_token_eot(m_context);
    const int tokenCount = whisper_full_n_tokens(m_context, segmentIndex);
    for (int k = 0; k < tokenCount; ++k) {
        const whisper_token_data data = whisper_full_get_token_data(m_context, segmentIndex, k);
        if (data.id >= eot) continue;
        const std::string tokenText = whisper_full_get_token_text(m_context, segmentIndex, k);
        if (tokenText.empty()) continue;
        if (pending.empty() || tokenText.front() == ' ') {
            pending.push_back({tokenText, data.t0, data.t1});
        } else {
            pending.back().text += tokenText;
            pending.back().rawT1 = data.t1;
        }
    }

    std::vector<TranscriptWord> words;
    if (pending.empty()) return words;

    // Los tiempos de token NO estan remapeados por el VAD (whisper.cpp 1.8.6
    // solo remapea t0/t1 de segmento): estan en el tiempo del audio ya sin
    // silencios. Se proyectan linealmente sobre el rango del segmento, que
    // si esta en tiempo original. Sin VAD la proyeccion es casi la identidad.
    // Es una aproximacion: alcanza para decidir de que lado de un cambio de
    // hablante cae cada palabra.
    const int64_t rawStart = pending.front().rawT0;
    const int64_t rawEnd = std::max(pending.back().rawT1, rawStart);
    const int64_t rawSpan = rawEnd - rawStart;
    const int64_t segSpan = std::max<int64_t>(segT1Cs - segT0Cs, 0);
    const size_t n = pending.size();
    words.reserve(n);
    for (size_t w = 0; w < n; ++w) {
        int64_t startCs;
        int64_t endCs;
        if (rawSpan > 0) {
            startCs = segT0Cs + (std::clamp(pending[w].rawT0, rawStart, rawEnd) - rawStart) * segSpan / rawSpan;
            endCs = segT0Cs + (std::clamp(pending[w].rawT1, rawStart, rawEnd) - rawStart) * segSpan / rawSpan;
        } else {
            // Sin tiempos de token utiles: reparto uniforme.
            startCs = segT0Cs + static_cast<int64_t>(w) * segSpan / static_cast<int64_t>(n);
            endCs = segT0Cs + static_cast<int64_t>(w + 1) * segSpan / static_cast<int64_t>(n);
        }
        const std::string text = trimSpaces(sanitizeUtf8(pending[w].text));
        if (text.empty()) continue;
        words.push_back({startCs * 10, std::max(startCs, endCs) * 10, text});
    }
    return words;
}

}  // namespace hermes::transcription
