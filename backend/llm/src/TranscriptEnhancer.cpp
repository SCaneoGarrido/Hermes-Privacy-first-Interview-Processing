#include "../include/TranscriptEnhancer.h"
#include "../include/TextUtils.h"
#include "../../api/include/logger.h"
#include "../../transcript/include/TranscriptRenderer.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

namespace hermes::llm {

namespace {

using hermes::transcript::SpeakerLabels;
using hermes::transcript::StructuredTranscript;

// Turnos por bloque en Fase 1. ~60 turnos cortos de entrevista real quedan
// comodamente por debajo de cualquier ventana de contexto razonable en un
// modelo local (ver Ollama Integration Strategy).
constexpr size_t TURNS_PER_CHUNK = 60;

// Caracteres por bloque en Fase 2/3 (sobre texto ya corregido).
constexpr size_t CHARS_PER_TEXT_CHUNK = 3000;

// Lineas ya corregidas del bloque anterior que se pasan como contexto al
// siguiente, para que la puntuacion/sentido no se corte en el limite.
constexpr size_t CONTINUITY_LINES = 3;

// Si la "correccion" de un turno cambia demasiado su largo, el modelo
// probablemente resumio, fusiono turnos o invento: se conserva el original.
// La correccion de ortografia/puntuacion no deberia mover el largo mas que esto.
constexpr double MIN_LENGTH_RATIO = 0.6;
constexpr double MAX_LENGTH_RATIO = 1.6;

using text::toLower;
using text::trim;

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            lines.push_back(trimmed);
        }
    }
    return lines;
}

// Divide texto en bloques de hasta maxChars, cortando en limites de linea
// (nunca a mitad de una oracion) para no romper el sentido dentro de un
// bloque.
std::vector<std::string> chunkTextByChars(const std::string& text, size_t maxChars) {
    std::vector<std::string> chunks;
    std::string current;
    for (const auto& line : splitLines(text)) {
        if (!current.empty() && current.size() + line.size() + 1 > maxChars) {
            chunks.push_back(current);
            current.clear();
        }
        current += line + "\n";
    }
    if (!current.empty()) {
        chunks.push_back(current);
    }
    return chunks;
}

std::string singleLine(const std::string& text) {
    std::string out = text;
    std::replace(out.begin(), out.end(), '\n', ' ');
    std::replace(out.begin(), out.end(), '\r', ' ');
    return trim(out);
}

// "[12] texto" -> {12, "texto"}. Tolera que el modelo repita la etiqueta de
// contexto "(Investigador)" al inicio del texto (se quita).
bool parseIndexedLine(const std::string& line, size_t& index, std::string& text) {
    if (line.size() < 3 || line[0] != '[') return false;
    const size_t close = line.find(']');
    if (close == std::string::npos || close == 1) return false;
    size_t value = 0;
    for (size_t i = 1; i < close; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(line[i]))) return false;
        value = value * 10 + static_cast<size_t>(line[i] - '0');
    }
    std::string rest = trim(line.substr(close + 1));
    if (!rest.empty() && rest[0] == '(') {
        const size_t closeParen = rest.find(')');
        if (closeParen != std::string::npos && closeParen < 60) {
            rest = trim(rest.substr(closeParen + 1));
        }
    }
    index = value;
    text = rest;
    return true;
}

bool plausibleCorrection(const std::string& original, const std::string& corrected) {
    if (corrected.empty()) return false;
    if (original.empty()) return false;
    const double ratio = static_cast<double>(corrected.size()) / static_cast<double>(original.size());
    // Turnos muy cortos ("Sí.", "Ya.") pueden cambiar de largo en proporcion
    // grande con una correccion legitima (agregar signos): no se filtran.
    if (original.size() < 15) return corrected.size() < 40;
    return ratio >= MIN_LENGTH_RATIO && ratio <= MAX_LENGTH_RATIO;
}

struct Entity {
    std::string text;  // casing original, la version mas larga vista
    std::string type;  // PERSONA | LUGAR | ORGANIZACION | OTRO
};

// Parsea la respuesta de la Fase 2a: una entidad por linea, formato
// "ENTIDAD|TIPO". Tolera lineas mal formadas (las ignora) en vez de
// tirar toda la extraccion por un renglon inesperado del LLM.
std::vector<Entity> parseEntities(const std::string& response) {
    std::vector<Entity> entities;
    for (const auto& line : splitLines(response)) {
        const size_t sep = line.find('|');
        if (sep == std::string::npos) continue;
        std::string text = trim(line.substr(0, sep));
        std::string type = trim(line.substr(sep + 1));
        if (text.empty() || type.empty()) continue;
        entities.push_back({text, type});
    }
    return entities;
}

}  // namespace

TranscriptEnhancer::TranscriptEnhancer(ILLMClient& client) : m_client(client) {}

EnhancementResult TranscriptEnhancer::enhance(StructuredTranscript& transcript,
                                               const SpeakerLabels& labels,
                                               bool enhanceTranscript,
                                               bool includeSummary,
                                               const ProgressCallback& onProgress) {
    EnhancementResult result;

    if (!enhanceTranscript) {
        if (includeSummary) {
            if (onProgress) onProgress("generando_resumen");
            try {
                result.summary = summarize(hermes::transcript::renderPlainText(transcript, labels, false));
            } catch (const std::exception& e) {
                result.failure = std::string("resumen: ") + e.what();
            }
        }
        return result;
    }

    if (onProgress) onProgress("corrigiendo_texto");
    std::vector<std::string> corrected = correctTurns(transcript, labels);
    for (size_t i = 0; i < transcript.turns.size(); ++i) {
        transcript.turns[i].text = std::move(corrected[i]);
    }
    result.corrected = true;
    result.correctedTranscript = hermes::transcript::renderPlainText(transcript, labels, false);

    if (onProgress) onProgress("anonimizando");
    try {
        std::vector<std::string> texts;
        texts.reserve(transcript.turns.size());
        for (const auto& turn : transcript.turns) texts.push_back(turn.text);
        std::vector<std::string> anonymized = anonymize(texts);
        for (size_t i = 0; i < transcript.turns.size(); ++i) {
            transcript.turns[i].text = std::move(anonymized[i]);
        }
        result.anonymized = true;
    } catch (const std::exception& e) {
        result.failure = std::string("anonimizacion: ") + e.what();
        return result;
    }

    if (includeSummary) {
        if (onProgress) onProgress("generando_resumen");
        try {
            result.summary = summarize(hermes::transcript::renderPlainText(transcript, labels, false));
        } catch (const std::exception& e) {
            result.failure = std::string("resumen: ") + e.what();
        }
    }
    return result;
}

std::vector<std::string> TranscriptEnhancer::correctTurns(const StructuredTranscript& transcript, const SpeakerLabels& labels) {
    static const std::string systemPrompt =
        "Sos un asistente que corrige transcripciones automaticas de entrevistas en espanol. "
        "Recibis lineas numeradas con el formato '[n] (Hablante) texto'. Tu tarea: corregir solo "
        "ortografia y puntuacion de cada linea (incluidos signos de pregunta). No cambies palabras por "
        "sinonimos, no agregues ni quites contenido, no resumas, no unas ni dividas lineas y no agregues "
        "comentarios propios. Devolve exactamente una linea por cada linea recibida, con el formato "
        "'[n] texto corregido' (mismo numero, sin el hablante), sin explicaciones adicionales.";

    const auto& turns = transcript.turns;
    std::vector<std::string> result;
    result.reserve(turns.size());
    for (const auto& turn : turns) result.push_back(turn.text);

    std::string continuity;
    const size_t totalChunks = (turns.size() + TURNS_PER_CHUNK - 1) / TURNS_PER_CHUNK;
    size_t chunkIndex = 0;
    size_t kept = 0;

    for (size_t start = 0; start < turns.size(); start += TURNS_PER_CHUNK) {
        const size_t end = std::min(start + TURNS_PER_CHUNK, turns.size());
        ++chunkIndex;
        log_event("[TranscriptEnhancer][correctTurns] Bloque " + std::to_string(chunkIndex) + "/" + std::to_string(totalChunks));

        std::string chunkText;
        for (size_t i = start; i < end; ++i) {
            const std::string label = labels.labelFor(turns[i].speaker);
            chunkText += "[" + std::to_string(i - start + 1) + "] " + (label.empty() ? "" : "(" + label + ") ") +
                         singleLine(turns[i].text) + "\n";
        }

        std::string userPrompt;
        if (!continuity.empty()) {
            userPrompt += "Ultimas lineas ya corregidas del fragmento anterior (solo contexto, no las devuelvas):\n";
            userPrompt += continuity + "\n";
        }
        userPrompt += "Lineas a corregir:\n" + chunkText;

        const std::string response = m_client.chat(systemPrompt, userPrompt);

        std::map<size_t, std::string> byIndex;
        for (const auto& line : splitLines(response)) {
            size_t index = 0;
            std::string text;
            if (parseIndexedLine(line, index, text) && index >= 1 && index <= end - start) {
                byIndex.emplace(index, text);
            }
        }

        continuity.clear();
        for (size_t i = start; i < end; ++i) {
            const auto it = byIndex.find(i - start + 1);
            const std::string original = singleLine(turns[i].text);
            if (it != byIndex.end() && plausibleCorrection(original, it->second)) {
                result[i] = it->second;
            } else {
                ++kept;  // se conserva el texto original del turno
            }
            if (i + CONTINUITY_LINES >= end) {
                continuity += result[i] + "\n";
            }
        }
    }

    if (kept > 0) {
        log_event("[TranscriptEnhancer][correctTurns] " + std::to_string(kept) + " de " + std::to_string(turns.size()) +
                  " turnos conservan el texto original (correccion ausente o descartada)");
    }
    return result;
}

std::vector<std::string> TranscriptEnhancer::anonymize(const std::vector<std::string>& texts) {
    static const std::string extractionSystemPrompt =
        "Sos un asistente que identifica informacion personal identificable (PII) en texto en espanol: "
        "nombres de personas, lugares, organizaciones o empresas. Analiza el texto y devolve una lista, "
        "una entidad por linea, en el formato exacto 'ENTIDAD|TIPO' donde TIPO es uno de: PERSONA, LUGAR, "
        "ORGANIZACION, OTRO. No repitas la misma entidad mas de una vez. Si no encontras ninguna entidad, "
        "no devuelvas nada. No agregues explicaciones ni texto fuera del formato pedido.";

    // Pasada 1: extraer entidades de cada bloque y acumular en una tabla
    // unica para toda la entrevista - la consistencia entre bloques
    // (mismo "Cristiano" -> mismo placeholder siempre) es el motivo de
    // separar esto en dos pasadas en vez de anonimizar bloque por bloque.
    // Se extrae sobre el texto de los turnos SIN etiquetas de hablante: las
    // etiquetas no son contenido y no deben terminar como entidad (ADR-017).
    std::string joined;
    for (const auto& text : texts) joined += singleLine(text) + "\n";

    std::map<std::string, Entity> uniqueEntities;  // key: toLower(entity.text)
    auto extractionChunks = chunkTextByChars(joined, CHARS_PER_TEXT_CHUNK);
    size_t extractionChunkIndex = 0;
    for (const auto& chunk : extractionChunks) {
        ++extractionChunkIndex;
        log_event("[TranscriptEnhancer][anonymize] Extraccion de entidades, bloque " +
                   std::to_string(extractionChunkIndex) + "/" + std::to_string(extractionChunks.size()));
        std::string response = m_client.chat(extractionSystemPrompt, chunk);
        for (const auto& entity : parseEntities(response)) {
            const std::string key = toLower(entity.text);
            auto it = uniqueEntities.find(key);
            if (it == uniqueEntities.end() || entity.text.size() > it->second.text.size()) {
                // Se queda con la variante mas larga vista (ej. "Cristiano
                // Ronaldo" sobre "Cristiano") para que el reemplazo de
                // texto mas abajo, ordenado de mas largo a mas corto, la
                // capture primero.
                uniqueEntities[key] = entity;
            }
        }
    }

    // Contadores de placeholder por tipo (PERSONA_1, PERSONA_2, LUGAR_1, ...).
    std::map<std::string, int> counters;
    std::vector<std::pair<std::string, std::string>> substitutions;  // (texto original, placeholder)
    for (const auto& [key, entity] : uniqueEntities) {
        int& counter = counters[entity.type];
        ++counter;
        substitutions.push_back({entity.text, "[" + entity.type + "_" + std::to_string(counter) + "]"});
    }

    // Pasada 2: sustitucion deterministica, turno por turno, de mas largo a
    // mas corto para no reemplazar "Cristiano" adentro de "Cristiano
    // Ronaldo" antes de que el nombre completo tenga su turno.
    std::sort(substitutions.begin(), substitutions.end(),
              [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });

    std::vector<std::string> anonymized = texts;
    for (auto& text : anonymized) {
        for (const auto& [original, placeholder] : substitutions) {
            size_t pos = 0;
            while ((pos = text.find(original, pos)) != std::string::npos) {
                text.replace(pos, original.size(), placeholder);
                pos += placeholder.size();
            }
        }
    }
    return anonymized;
}

std::string TranscriptEnhancer::summarize(const std::string& text) {
    static const std::string mapSystemPrompt =
        "Sos un asistente que resume fragmentos de entrevistas en espanol de forma breve (2-3 oraciones), "
        "manteniendo los hechos y sin agregar interpretaciones propias.";
    static const std::string reduceSystemPrompt =
        "Sos un asistente que combina resumenes parciales de distintos fragmentos de una misma entrevista, "
        "en orden cronologico, en un resumen final coherente y conciso (un parrafo), sin repetir informacion.";

    auto chunks = chunkTextByChars(text, CHARS_PER_TEXT_CHUNK);

    // Map: un resumen breve por bloque.
    std::string combinedSummaries;
    size_t chunkIndex = 0;
    for (const auto& chunk : chunks) {
        ++chunkIndex;
        log_event("[TranscriptEnhancer][summarize] Resumen (map), bloque " +
                   std::to_string(chunkIndex) + "/" + std::to_string(chunks.size()));
        combinedSummaries += trim(m_client.chat(mapSystemPrompt, chunk)) + "\n";
    }

    // Documento corto (una entrevista de un solo bloque, por ejemplo): el
    // "resumen de resumenes" de un unico resumen es el mismo resumen, no
    // hace falta una llamada extra a Ollama.
    if (chunks.size() <= 1) {
        return trim(combinedSummaries);
    }

    // Reduce: combinar los resumenes parciales en uno solo.
    log_event("[TranscriptEnhancer][summarize] Combinando resumenes (reduce)");
    return trim(m_client.chat(reduceSystemPrompt, combinedSummaries));
}

}  // namespace hermes::llm
