#include "../include/TranscriptEnhancer.h"
#include "../../api/include/logger.h"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>

namespace hermes::llm {

namespace {

// Segmentos por bloque en Fase 1 (sobre segments crudos de whisper).
// ~60 segmentos cortos de entrevista real quedan comodamente por debajo de
// cualquier ventana de contexto razonable en un modelo local (ver Ollama
// Integration Strategy).
constexpr size_t SEGMENTS_PER_CHUNK = 60;

// Caracteres por bloque en Fase 2/3 (sobre texto ya corregido, no
// segmentos crudos) - mas robusto que contar lineas, porque Fase 1 puede
// fusionar varios segmentos crudos en menos lineas de dialogo.
constexpr size_t CHARS_PER_TEXT_CHUNK = 3000;

// Lineas de continuidad que se le pasan al siguiente bloque de Fase 1 para
// que el modelo sepa quien hablo ultimo y no reinicie la suposicion de que
// el primer turno es "Investigador" en cada bloque.
constexpr int CONTINUITY_LINES = 3;

std::string trim(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

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

// Ultimas N lineas no vacias de un texto, unidas de nuevo con salto de
// linea - usado como contexto de continuidad entre bloques.
std::string lastLines(const std::string& text, int n) {
    auto lines = splitLines(text);
    const size_t start = lines.size() > static_cast<size_t>(n) ? lines.size() - n : 0;
    std::string result;
    for (size_t i = start; i < lines.size(); ++i) {
        result += lines[i] + "\n";
    }
    return result;
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

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Distancia de edicion entre dos strings, usada para mapear variantes de
// etiqueta a la canonica mas parecida (ver abajo).
size_t levenshtein(const std::string& a, const std::string& b) {
    std::vector<size_t> prev(b.size() + 1), curr(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (size_t i = 1; i <= a.size(); ++i) {
        curr[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            const size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

const std::vector<std::string> CANONICAL_LABELS_LOWER = {"investigador", "entrevistado"};
const std::vector<std::string> CANONICAL_LABELS = {"Investigador", "Entrevistado"};

// Maxima distancia de edicion para considerar que la primera palabra de una
// linea es una variante inventada de etiqueta (ej. "Investigado:",
// "Entrevistador:", ver Common Mistakes en Ollama Integration Strategy) en
// vez de contenido normal que no deberia tocarse.
constexpr size_t LABEL_MAX_EDIT_DISTANCE = 3;

// Si la linea empieza con algo que se parece a "Investigador:" o
// "Entrevistado:" pero no es exactamente eso, la reemplaza por la etiqueta
// canonica mas cercana. El modelo a veces no respeta el formato exacto
// pedido en el prompt; esto normaliza sin volver a invocar a Ollama.
std::string normalizeSpeakerLabel(const std::string& line) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) return line;

    const std::string labelLower = toLower(line.substr(0, colon));
    if (labelLower == CANONICAL_LABELS_LOWER[0] || labelLower == CANONICAL_LABELS_LOWER[1]) {
        return line;
    }

    size_t bestIdx = 0;
    size_t bestDist = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < CANONICAL_LABELS_LOWER.size(); ++i) {
        const size_t dist = levenshtein(labelLower, CANONICAL_LABELS_LOWER[i]);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    if (bestDist > LABEL_MAX_EDIT_DISTANCE) return line;
    return CANONICAL_LABELS[bestIdx] + line.substr(colon);
}

}  // namespace

TranscriptEnhancer::TranscriptEnhancer(ILLMClient& client) : m_client(client) {}

EnhancementResult TranscriptEnhancer::enhance(const std::vector<hermes::transcription::TranscriptSegment>& segments,
                                               bool includeSummary,
                                               const ProgressCallback& onProgress) {
    EnhancementResult result;

    if (onProgress) onProgress("corrigiendo_texto");
    result.correctedTranscript = correctAndStructure(segments);

    if (onProgress) onProgress("anonimizando");
    result.anonymizedTranscript = anonymize(result.correctedTranscript);

    if (includeSummary) {
        if (onProgress) onProgress("generando_resumen");
        result.summary = summarize(result.anonymizedTranscript);
    }
    return result;
}

std::string TranscriptEnhancer::correctAndStructure(const std::vector<hermes::transcription::TranscriptSegment>& segments) {
    static const std::string systemPrompt =
        "Sos un asistente que corrige transcripciones automaticas de entrevistas en espanol. "
        "Recibis fragmentos de audio transcripto (con errores de puntuacion y sin indicar quien habla). "
        "Tu tarea: 1) Corregir ortografia y puntuacion. 2) Etiquetar cada linea con 'Investigador:' o "
        "'Entrevistado:' segun el contexto gramatical y semantico (son dos personas alternando turnos de "
        "habla). No agregues ni quites contenido, no resumas, no agregues comentarios propios. Devolve "
        "unicamente las lineas corregidas y etiquetadas, una por linea, sin explicaciones adicionales.";

    std::string result;
    std::string continuity;
    const size_t totalChunks = (segments.size() + SEGMENTS_PER_CHUNK - 1) / SEGMENTS_PER_CHUNK;
    size_t chunkIndex = 0;

    for (size_t start = 0; start < segments.size(); start += SEGMENTS_PER_CHUNK) {
        const size_t end = std::min(start + SEGMENTS_PER_CHUNK, segments.size());
        ++chunkIndex;
        log_event("[TranscriptEnhancer][correctAndStructure] Bloque " + std::to_string(chunkIndex) + "/" + std::to_string(totalChunks));

        std::string chunkText;
        for (size_t i = start; i < end; ++i) {
            chunkText += trim(segments[i].text) + "\n";
        }

        std::string userPrompt;
        if (!continuity.empty()) {
            userPrompt += "Ultimas lineas ya procesadas del fragmento anterior (para continuidad, no las repitas):\n";
            userPrompt += continuity + "\n";
        }
        userPrompt += "Fragmento a procesar:\n" + chunkText;

        std::string chunkResult = trim(m_client.chat(systemPrompt, userPrompt));

        std::string normalizedChunk;
        for (const auto& line : splitLines(chunkResult)) {
            normalizedChunk += normalizeSpeakerLabel(line) + "\n";
        }
        chunkResult = trim(normalizedChunk);

        result += chunkResult + "\n";
        continuity = lastLines(chunkResult, CONTINUITY_LINES);
    }

    return result;
}

std::string TranscriptEnhancer::anonymize(const std::string& correctedText) {
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
    std::map<std::string, Entity> uniqueEntities;  // key: toLower(entity.text)

    auto extractionChunks = chunkTextByChars(correctedText, CHARS_PER_TEXT_CHUNK);
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

    // Pasada 2: sustitucion deterministica de texto plano, de mas largo a
    // mas corto para no reemplazar "Cristiano" adentro de "Cristiano
    // Ronaldo" antes de que el nombre completo tenga su turno.
    std::sort(substitutions.begin(), substitutions.end(),
              [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });

    std::string anonymized = correctedText;
    for (const auto& [original, placeholder] : substitutions) {
        size_t pos = 0;
        while ((pos = anonymized.find(original, pos)) != std::string::npos) {
            anonymized.replace(pos, original.size(), placeholder);
            pos += placeholder.size();
        }
    }

    return anonymized;
}

std::string TranscriptEnhancer::summarize(const std::string& anonymizedText) {
    static const std::string mapSystemPrompt =
        "Sos un asistente que resume fragmentos de entrevistas en espanol de forma breve (2-3 oraciones), "
        "manteniendo los hechos y sin agregar interpretaciones propias.";
    static const std::string reduceSystemPrompt =
        "Sos un asistente que combina resumenes parciales de distintos fragmentos de una misma entrevista, "
        "en orden cronologico, en un resumen final coherente y conciso (un parrafo), sin repetir informacion.";

    auto chunks = chunkTextByChars(anonymizedText, CHARS_PER_TEXT_CHUNK);

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
