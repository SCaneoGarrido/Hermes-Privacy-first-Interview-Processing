#include "../include/GlossarySanitizer.h"
#include "../include/TextUtils.h"
#include "../../api/include/logger.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <map>

namespace hermes::llm {

namespace {

using text::levenshtein;
using text::toLower;
using text::trim;

// Segmentos por bloque: ~40 lineas cortas de whisper + el glosario quedan
// muy por debajo de num_ctx, y un bloque chico acota el alcance de cada
// reemplazo (solo se aplica dentro del bloque donde se detecto).
constexpr size_t SEGMENTS_PER_BLOCK = 40;

// La respuesta es una lista corta de pares; si el modelo se extiende mas
// de esto, algo anda mal y se corta.
constexpr int MAX_RESPONSE_TOKENS = 512;

// Plausibilidad fonetica: la variante y el termino (normalizados) no pueden
// diferir en mas de esta fraccion del largo mayor. Descarta propuestas del
// tipo "paciente" -> "PET" que el contexto podria sugerir al modelo.
constexpr double MAX_RELATIVE_DISTANCE = 0.5;

constexpr size_t MAX_ORIGINAL_LENGTH = 80;

const char* SYSTEM_PROMPT =
    "Sos un asistente que revisa transcripciones automaticas de entrevistas en espanol. "
    "Recibis un glosario de terminos correctos del tema de la entrevista y un fragmento transcripto. "
    "Tu tarea: encontrar palabras o frases del fragmento que sean transcripciones erroneas de algun "
    "termino del glosario (suenan parecido y el contexto lo confirma). Para cada una devolve "
    "'original', copiado exactamente como aparece en el fragmento, y 'termino', copiado exactamente "
    "del glosario. No propongas cambios para palabras que no correspondan a un termino del glosario. "
    "Si no hay ninguna, devolve una lista vacia.";

const char* RESPONSE_SCHEMA = R"({
  "type": "object",
  "properties": {
    "correcciones": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "original": {"type": "string"},
          "termino": {"type": "string"}
        },
        "required": ["original", "termino"]
      }
    }
  },
  "required": ["correcciones"]
})";

// Los bytes UTF-8 no ASCII cuentan como parte de palabra: asi "GES" no
// matchea dentro de "gestión" ni "ñ" corta una palabra en dos.
bool isWordByte(unsigned char c) {
    return c >= 0x80 || std::isalnum(c);
}

// Reemplaza `from` por `to` solo como palabra completa. Devuelve cuantas
// veces lo hizo.
size_t replaceWholeWords(std::string& text, const std::string& from, const std::string& to) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        const size_t end = pos + from.size();
        const bool startOk = pos == 0 || !isWordByte(static_cast<unsigned char>(text[pos - 1]));
        const bool endOk = end == text.size() || !isWordByte(static_cast<unsigned char>(text[end]));
        if (startOk && endOk) {
            text.replace(pos, from.size(), to);
            pos += to.size();
            ++count;
        } else {
            pos = end;
        }
    }
    return count;
}

// Forma comparable para la distancia fonetica: minusculas y sin espacios ni
// puntuacion ("Sam borja" ~ "San Borja").
std::string normalizeForDistance(const std::string& text) {
    std::string normalized;
    for (unsigned char c : toLower(text)) {
        if (isWordByte(c)) normalized += static_cast<char>(c);
    }
    return normalized;
}

bool isPhoneticallyPlausible(const std::string& original, const std::string& term) {
    const std::string a = normalizeForDistance(original);
    const std::string b = normalizeForDistance(term);
    if (a.empty() || b.empty()) return false;
    const size_t longest = std::max(a.size(), b.size());
    return static_cast<double>(levenshtein(a, b)) <= MAX_RELATIVE_DISTANCE * static_cast<double>(longest);
}

struct Proposal {
    std::string original;
    std::string term;  // forma canonica del glosario
};

}  // namespace

GlossarySanitizer::GlossarySanitizer(ILLMClient& client) : m_client(client) {}

SanitizationResult GlossarySanitizer::sanitize(const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                               const std::vector<std::string>& glossary) {
    SanitizationResult result;
    result.segments = segments;
    if (glossary.empty() || segments.empty()) {
        return result;
    }

    std::map<std::string, std::string> canonicalByLower;  // toLower(termino) -> forma del usuario
    std::string glossaryList;
    for (const auto& term : glossary) {
        canonicalByLower.emplace(toLower(term), term);
        glossaryList += "- " + term + "\n";
    }

    const size_t totalBlocks = (segments.size() + SEGMENTS_PER_BLOCK - 1) / SEGMENTS_PER_BLOCK;
    for (size_t start = 0, blockIndex = 1; start < segments.size(); start += SEGMENTS_PER_BLOCK, ++blockIndex) {
        const size_t end = std::min(start + SEGMENTS_PER_BLOCK, segments.size());
        const std::string blockTag = "bloque " + std::to_string(blockIndex) + "/" + std::to_string(totalBlocks);

        std::string fragment;
        for (size_t i = start; i < end; ++i) {
            fragment += trim(result.segments[i].text) + "\n";
        }

        const std::string response = m_client.chatStructured(
            SYSTEM_PROMPT, "Glosario:\n" + glossaryList + "\nFragmento:\n" + fragment, RESPONSE_SCHEMA, MAX_RESPONSE_TOKENS);

        nlohmann::json parsed;
        try {
            parsed = nlohmann::json::parse(response);
        } catch (const nlohmann::json::parse_error&) {
            log_event("[GlossarySanitizer][sanitize] Respuesta no parseable, " + blockTag + " queda sin sanitizar");
            continue;
        }
        if (!parsed.contains("correcciones") || !parsed["correcciones"].is_array()) {
            log_event("[GlossarySanitizer][sanitize] Respuesta sin 'correcciones', " + blockTag + " queda sin sanitizar");
            continue;
        }

        // Validacion determinista: el modelo propone, esto decide.
        std::vector<Proposal> accepted;
        size_t discarded = 0;
        for (const auto& item : parsed["correcciones"]) {
            if (!item.contains("original") || !item.contains("termino") ||
                !item["original"].is_string() || !item["termino"].is_string()) {
                ++discarded;
                continue;
            }
            const std::string original = trim(item["original"].get<std::string>());
            const auto canonical = canonicalByLower.find(toLower(trim(item["termino"].get<std::string>())));

            const bool valid = canonical != canonicalByLower.end()                        // 1. es un termino del glosario
                && !original.empty() && original.size() <= MAX_ORIGINAL_LENGTH
                && original != canonical->second                                          // 3. hay algo que cambiar
                && canonicalByLower.find(toLower(original)) == canonicalByLower.end()      //    y no es otro termino
                && isPhoneticallyPlausible(original, canonical->second);                  // 4. suena parecido
            if (!valid) {
                ++discarded;
                continue;
            }
            accepted.push_back({original, canonical->second});
        }

        // Mas largas primero: "sin tiraama" antes que "tiraama".
        std::sort(accepted.begin(), accepted.end(),
                  [](const Proposal& a, const Proposal& b) { return a.original.size() > b.original.size(); });

        size_t applied = 0;
        for (const auto& proposal : accepted) {
            bool found = false;  // 2. tiene que aparecer en el bloque como palabra completa
            for (size_t i = start; i < end; ++i) {
                if (replaceWholeWords(result.segments[i].text, proposal.original, proposal.term) > 0) {
                    result.changes.push_back({i + 1, proposal.original, proposal.term});
                    found = true;
                }
            }
            if (found) {
                ++applied;
            } else {
                ++discarded;
            }
        }

        log_event("[GlossarySanitizer][sanitize] " + blockTag + ": " + std::to_string(applied) +
                  " correcciones aplicadas, " + std::to_string(discarded) + " descartadas");
    }

    return result;
}

}  // namespace hermes::llm
