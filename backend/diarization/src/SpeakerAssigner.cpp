#include "../include/SpeakerAssigner.h"
#include "../../api/include/logger.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <optional>

namespace hermes::diarization {

namespace {

using hermes::transcript::SpeakerRole;
using hermes::transcript::TranscriptTurn;
using hermes::transcription::TranscriptSegment;
using hermes::transcription::TranscriptWord;

// Cobertura minima de un hablante para asignarle el segmento entero...
constexpr double DOMINANT_SHARE = 0.7;
// ...salvo que otro hablante ocupe al menos esto dentro del segmento: los
// segmentos de whisper llegan a 30s, y una intervencion corta del otro
// hablante (una pregunta de 3s dentro de una respuesta larga) queda por
// debajo del 30% pero es un turno real.
constexpr int64_t SECOND_SPEAKER_MIN_MS = 2000;
// Tramos de menos palabras que esto se unen al tramo vecino al dividir.
constexpr size_t MIN_RUN_WORDS = 2;
// Diferencia de proporcion de preguntas por debajo de la cual se considera
// empate y se decide por tiempo de habla.
constexpr double QUESTION_SCORE_TIE = 0.05;
// Un cluster cuenta como una persona si tiene al menos esta parte del tiempo
// total de habla y esta cantidad de fragmentos. Validado en las entrevistas 6
// y 7: deja fuera clusters de ruido/asentimientos (decenas de segundos en una
// entrevista de 30-50 min) y conserva los que concentran las preguntas.
constexpr double RELEVANT_MIN_TALK_SHARE = 0.03;
constexpr size_t RELEVANT_MIN_PIECES = 3;
// Los tiempos por palabra (proyectados, ver WhisperTranscriber) y los bordes
// de la diarizacion tienen un error de 1-4 palabras: un corte crudo cae a
// mitad de frase ("Apague la camara por | subir la señal"). Cada corte se
// mueve al fin de oracion mas cercano dentro de esta distancia (en palabras).
constexpr size_t SNAP_MAX_WORDS = 4;
// Sin fin de oracion cerca, un corte a esta distancia o menos del borde del
// segmento se descarta (el segmento entero queda de un solo hablante).
constexpr size_t EDGE_MAX_WORDS = 2;

bool endsSentence(const std::string& word) {
    if (word.empty()) return false;
    const char last = word.back();
    return last == '.' || last == '?' || last == '!';
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

// ms de solapamiento de [startMs, endMs) con cada hablante.
std::map<int, int64_t> overlapBySpeaker(int64_t startMs, int64_t endMs, const std::vector<SpeakerTurn>& turns) {
    std::map<int, int64_t> overlap;
    for (const auto& turn : turns) {
        if (turn.startMs >= endMs) break;  // ordenados por inicio
        const int64_t from = std::max(startMs, turn.startMs);
        const int64_t to = std::min(endMs, turn.endMs);
        if (to > from) overlap[turn.speakerId] += to - from;
    }
    return overlap;
}

// Hablante en el instante t: el tramo que lo contiene (si hay habla
// superpuesta, el tramo mas largo: los fragmentos cortos superpuestos suelen
// ser asentimientos o ruido), o si cae en un hueco (silencio, o tramo
// descartado por corto) el tramo mas cercano.
std::optional<int> speakerAt(int64_t t, const std::vector<SpeakerTurn>& turns) {
    std::optional<int> containing;
    int64_t containingLength = -1;
    std::optional<int> nearest;
    int64_t bestDistance = std::numeric_limits<int64_t>::max();
    for (const auto& turn : turns) {
        if (turn.startMs > t && containing) break;
        if (turn.startMs <= t && t < turn.endMs) {
            if (turn.endMs - turn.startMs > containingLength) {
                containingLength = turn.endMs - turn.startMs;
                containing = turn.speakerId;
            }
            continue;
        }
        const int64_t distance = t < turn.startMs ? turn.startMs - t : t - turn.endMs;
        if (distance < bestDistance) {
            bestDistance = distance;
            nearest = turn.speakerId;
        }
        if (turn.startMs > t && distance > bestDistance) break;
    }
    return containing ? containing : nearest;
}

std::optional<int> dominantSpeaker(const std::map<int, int64_t>& overlap, int64_t& dominantMs, int64_t& totalMs) {
    std::optional<int> best;
    dominantMs = 0;
    totalMs = 0;
    for (const auto& [speaker, ms] : overlap) {
        totalMs += ms;
        if (ms > dominantMs) {
            dominantMs = ms;
            best = speaker;
        }
    }
    return best;
}

// Fragmento con el cluster de diarizacion (aun sin rol).
struct Piece {
    int64_t startMs;
    int64_t endMs;
    std::optional<int> speakerId;
    std::string text;
};

std::string joinWords(const std::vector<TranscriptWord>& words, size_t from, size_t to) {
    std::string text;
    for (size_t i = from; i < to; ++i) {
        if (!text.empty()) text += ' ';
        text += words[i].text;
    }
    return text;
}

// Divide un segmento que cruza un cambio de hablante oracion por oracion:
// cada oracion va al hablante con mas tiempo dentro de su intervalo. Una
// persona casi nunca cambia a mitad de oracion, y decidir por oracion absorbe
// el error de 1-4 palabras de los tiempos por palabra y de los bordes de la
// diarizacion. nullopt si el segmento tiene una sola oracion (sin puntuacion
// interna): ahi se decide palabra por palabra (splitByWords).
std::optional<std::vector<Piece>> splitBySentences(const TranscriptSegment& segment, const std::vector<SpeakerTurn>& turns) {
    const auto& words = segment.words;
    std::vector<std::pair<size_t, size_t>> sentences;  // [from, to)
    size_t from = 0;
    for (size_t i = 0; i < words.size(); ++i) {
        if (endsSentence(words[i].text) || i + 1 == words.size()) {
            sentences.push_back({from, i + 1});
            from = i + 1;
        }
    }
    if (sentences.size() < 2) return std::nullopt;

    std::vector<std::optional<int>> speakers;
    speakers.reserve(sentences.size());
    for (const auto& [first, last] : sentences) {
        const int64_t start = words[first].startMs;
        const int64_t end = std::max(words[last - 1].endMs, start + 1);
        int64_t dominantMs = 0;
        int64_t totalMs = 0;
        const auto dominant = dominantSpeaker(overlapBySpeaker(start, end, turns), dominantMs, totalMs);
        speakers.push_back(totalMs > 0 ? dominant : speakerAt((start + end) / 2, turns));
    }

    std::vector<Piece> pieces;
    for (size_t s = 0; s < sentences.size(); ++s) {
        const size_t first = sentences[s].first;
        if (!pieces.empty() && pieces.back().speakerId == speakers[s]) {
            pieces.back().text += " " + joinWords(words, first, sentences[s].second);
            continue;
        }
        if (!pieces.empty()) pieces.back().endMs = std::max(pieces.back().startMs, words[first].startMs);
        pieces.push_back({pieces.empty() ? segment.startMs : words[first].startMs, segment.endMs, speakers[s],
                          joinWords(words, first, sentences[s].second)});
    }
    if (pieces.size() == 1) {
        pieces[0].text = trim(segment.text);  // conserva el texto original tal cual
    }
    return pieces;
}

// Divide un segmento que cruza un cambio de hablante, palabra por palabra.
std::vector<Piece> splitByWords(const TranscriptSegment& segment, const std::vector<SpeakerTurn>& turns) {
    const auto& words = segment.words;
    std::vector<std::optional<int>> wordSpeaker(words.size());
    for (size_t i = 0; i < words.size(); ++i) {
        wordSpeaker[i] = speakerAt((words[i].startMs + words[i].endMs) / 2, turns);
    }

    // Tramos [from, to) de palabras consecutivas con el mismo hablante.
    struct Run {
        size_t from;
        size_t to;
        std::optional<int> speaker;
    };
    std::vector<Run> runs;
    for (size_t i = 0; i < words.size(); ++i) {
        if (runs.empty() || runs.back().speaker != wordSpeaker[i]) {
            runs.push_back({i, i + 1, wordSpeaker[i]});
        } else {
            runs.back().to = i + 1;
        }
    }

    // Absorbe tramos cortos en el vecino (el anterior si existe) y vuelve a
    // fusionar los que quedan contiguos con el mismo hablante.
    bool changed = true;
    while (changed && runs.size() > 1) {
        changed = false;
        for (size_t r = 0; r < runs.size(); ++r) {
            if (runs[r].to - runs[r].from >= MIN_RUN_WORDS) continue;
            if (r > 0) {
                runs[r - 1].to = runs[r].to;
            } else {
                runs[r + 1].from = runs[r].from;
            }
            runs.erase(runs.begin() + static_cast<long>(r));
            for (size_t k = 1; k < runs.size();) {
                if (runs[k].speaker == runs[k - 1].speaker) {
                    runs[k - 1].to = runs[k].to;
                    runs.erase(runs.begin() + static_cast<long>(k));
                } else {
                    ++k;
                }
            }
            changed = true;
            break;
        }
    }

    // Ajusta cada corte (inicio de un tramo) a un fin de oracion cercano.
    if (runs.size() > 1) {
        const size_t n = words.size();
        std::vector<Run> snapped;
        snapped.push_back(runs[0]);
        for (size_t r = 1; r < runs.size(); ++r) {
            const size_t cut = runs[r].from;
            std::optional<size_t> best;
            size_t bestDistance = SNAP_MAX_WORDS + 1;
            const size_t lo = cut > SNAP_MAX_WORDS ? cut - SNAP_MAX_WORDS : 1;
            const size_t hi = std::min(n - 1, cut + SNAP_MAX_WORDS);
            for (size_t k = std::max<size_t>(lo, 1); k <= hi; ++k) {
                if (!endsSentence(words[k - 1].text)) continue;
                const size_t distance = k > cut ? k - cut : cut - k;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = k;
                }
            }
            Run& prev = snapped.back();
            size_t newCut = best.value_or(cut);
            if (!best && (cut <= EDGE_MAX_WORDS || n - cut <= EDGE_MAX_WORDS)) {
                // Sin fin de oracion y pegado al borde: no se divide. Queda el
                // hablante del tramo con mas palabras.
                if (runs[r].to - runs[r].from > prev.to - prev.from) prev.speaker = runs[r].speaker;
                prev.to = runs[r].to;
                continue;
            }
            // El corte movido no puede cruzar el inicio del tramo anterior ni
            // el final del actual.
            newCut = std::clamp(newCut, prev.from + 1, runs[r].to - 1 >= prev.from + 1 ? runs[r].to - 1 : prev.from + 1);
            prev.to = newCut;
            snapped.push_back({newCut, runs[r].to, runs[r].speaker});
        }
        runs.clear();
        for (const auto& run : snapped) {
            if (run.to <= run.from) continue;
            if (!runs.empty() && runs.back().speaker == run.speaker) {
                runs.back().to = run.to;
            } else {
                runs.push_back(run);
            }
        }
    }

    std::vector<Piece> pieces;
    if (runs.size() <= 1) {
        // Termino siendo de un solo hablante: se conserva el texto original
        // del segmento (con su espaciado/puntuacion tal cual).
        pieces.push_back({segment.startMs, segment.endMs, runs.empty() ? std::nullopt : runs[0].speaker, trim(segment.text)});
        return pieces;
    }
    for (size_t r = 0; r < runs.size(); ++r) {
        const int64_t start = r == 0 ? segment.startMs : words[runs[r].from].startMs;
        const int64_t end = r + 1 == runs.size() ? segment.endMs : words[runs[r + 1].from].startMs;
        pieces.push_back({start, std::max(start, end), runs[r].speaker, joinWords(words, runs[r].from, runs[r].to)});
    }
    return pieces;
}

bool isQuestion(const std::string& text) {
    return text.find('?') != std::string::npos || text.find("\xC2\xBF") != std::string::npos;  // "¿"
}

struct ClusterStats {
    int64_t talkMs = 0;
    size_t pieces = 0;
    size_t questions = 0;
    int64_t firstMs = std::numeric_limits<int64_t>::max();

    double questionScore() const { return pieces == 0 ? 0.0 : static_cast<double>(questions) / static_cast<double>(pieces); }
};

// Con clustering por umbral (ver SherpaOnnxDiarizer) la voz de una misma
// persona puede quedar repartida en varios clusters, y aparecen clusters
// chicos de ruido, risas o asentimientos. Un cluster es relevante (una
// persona) si tiene suficiente tiempo de habla y tramos; los tramos de
// clusters no relevantes se reasignan al cluster relevante del tramo mas
// cercano en el tiempo (un fragmento suelto pegado a un turno es casi
// siempre de quien habla en ese turno). Devuelve cuantos clusters relevantes hay.
size_t keepRelevantClusters(std::vector<SpeakerTurn>& turns) {
    std::map<int, int64_t> talk;
    std::map<int, size_t> count;
    int64_t total = 0;
    for (const auto& turn : turns) {
        talk[turn.speakerId] += turn.endMs - turn.startMs;
        ++count[turn.speakerId];
        total += turn.endMs - turn.startMs;
    }
    std::map<int, bool> relevant;
    size_t relevantCount = 0;
    for (const auto& [id, ms] : talk) {
        relevant[id] = count[id] >= RELEVANT_MIN_PIECES && static_cast<double>(ms) >= RELEVANT_MIN_TALK_SHARE * static_cast<double>(total);
        if (relevant[id]) ++relevantCount;
    }
    if (relevantCount == 0) return 0;

    std::vector<SpeakerTurn> original = turns;
    for (auto& turn : turns) {
        if (relevant[turn.speakerId]) continue;
        int64_t bestGap = std::numeric_limits<int64_t>::max();
        int64_t bestLength = -1;
        for (const auto& other : original) {
            if (!relevant[other.speakerId]) continue;
            const int64_t gap = std::max<int64_t>({0, other.startMs - turn.endMs, turn.startMs - other.endMs});
            const int64_t length = other.endMs - other.startMs;
            if (gap < bestGap || (gap == bestGap && length > bestLength)) {
                bestGap = gap;
                bestLength = length;
                turn.speakerId = other.speakerId;
            }
        }
    }
    return relevantCount;
}

std::string formatScore(double value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.3f", value);
    return buf;
}

}  // namespace

std::vector<TranscriptTurn> assignSpeakers(const std::vector<TranscriptSegment>& segments, const std::vector<SpeakerTurn>& speakerTurns) {
    std::vector<SpeakerTurn> turns = speakerTurns;
    std::sort(turns.begin(), turns.end(), [](const SpeakerTurn& a, const SpeakerTurn& b) { return a.startMs < b.startMs; });
    const size_t relevantCount = keepRelevantClusters(turns);

    std::vector<Piece> pieces;
    pieces.reserve(segments.size());
    size_t splitCount = 0;
    for (const auto& segment : segments) {
        if (trim(segment.text).empty()) continue;
        const auto overlap = overlapBySpeaker(segment.startMs, segment.endMs, turns);
        int64_t dominantMs = 0;
        int64_t totalMs = 0;
        const auto dominant = dominantSpeaker(overlap, dominantMs, totalMs);
        const int64_t duration = std::max<int64_t>(segment.endMs - segment.startMs, 1);

        int64_t secondMs = 0;
        for (const auto& [speaker, ms] : overlap) {
            if (speaker != dominant && ms > secondMs) secondMs = ms;
        }
        const bool mixed = static_cast<double>(dominantMs) < DOMINANT_SHARE * static_cast<double>(duration) ||
                           secondMs >= SECOND_SPEAKER_MIN_MS;
        if (overlap.size() > 1 && mixed && segment.words.size() >= 2 * MIN_RUN_WORDS) {
            auto bySentence = splitBySentences(segment, turns);
            auto split = bySentence ? std::move(*bySentence) : splitByWords(segment, turns);
            if (split.size() > 1) ++splitCount;
            pieces.insert(pieces.end(), split.begin(), split.end());
            continue;
        }

        std::optional<int> speaker = totalMs > 0 ? dominant : speakerAt((segment.startMs + segment.endMs) / 2, turns);
        pieces.push_back({segment.startMs, segment.endMs, speaker, trim(segment.text)});
    }

    std::map<int, ClusterStats> stats;
    for (const auto& piece : pieces) {
        if (!piece.speakerId) continue;
        auto& s = stats[*piece.speakerId];
        s.talkMs += piece.endMs - piece.startMs;
        ++s.pieces;
        if (isQuestion(piece.text)) ++s.questions;
        s.firstMs = std::min(s.firstMs, piece.startMs);
    }

    std::vector<TranscriptTurn> result;
    result.reserve(pieces.size());

    // Los tramos ya solo traen clusters relevantes (keepRelevantClusters).
    std::vector<int> relevant;
    for (const auto& [id, s] : stats) relevant.push_back(id);

    if (relevant.size() < 2) {
        log_event("[SpeakerAssigner][assignSpeakers] La diarizacion encontro " + std::to_string(relevantCount) +
                  " hablante(s) relevante(s); no se puede distinguir investigador de sujeto, se entrega sin hablantes");
        for (auto& piece : pieces) {
            result.push_back({piece.startMs, piece.endMs, std::nullopt, std::move(piece.text)});
        }
        return result;
    }

    // Investigador: el cluster relevante que mas pregunta. Referencia del
    // sujeto: el que mas habla entre los demas.
    std::sort(relevant.begin(), relevant.end(), [&](int a, int b) { return stats[a].questionScore() > stats[b].questionScore(); });
    int interviewer = relevant[0];
    int subject = -1;
    for (size_t i = 1; i < relevant.size(); ++i) {
        if (subject < 0 || stats[relevant[i]].talkMs > stats[subject].talkMs) subject = relevant[i];
    }
    std::string reason = "mayor proporcion de preguntas";
    if (std::abs(stats[interviewer].questionScore() - stats[subject].questionScore()) < QUESTION_SCORE_TIE) {
        const int a = interviewer;
        const int b = subject;
        if (stats[a].talkMs != stats[b].talkMs) {
            interviewer = stats[a].talkMs < stats[b].talkMs ? a : b;
            reason = "empate en preguntas, habla menos";
        } else {
            interviewer = stats[a].firstMs <= stats[b].firstMs ? a : b;
            reason = "empate, habla primero";
        }
        subject = interviewer == a ? b : a;
    }
    const double interviewerScore = stats[interviewer].questionScore();
    const double subjectScore = stats[subject].questionScore();

    // Rol de cada cluster relevante: el investigador, el sujeto de referencia,
    // y los demas al rol cuyo perfil de preguntas se parece mas (normalmente
    // la misma persona repartida en otro cluster).
    std::map<int, SpeakerRole> roleOf;
    int64_t interviewerTalk = 0;
    int64_t subjectTalk = 0;
    for (int id : relevant) {
        SpeakerRole role;
        if (id == interviewer) {
            role = SpeakerRole::Interviewer;
        } else if (id == subject) {
            role = SpeakerRole::Subject;
        } else {
            const double score = stats[id].questionScore();
            role = std::abs(score - interviewerScore) < std::abs(score - subjectScore) ? SpeakerRole::Interviewer : SpeakerRole::Subject;
        }
        roleOf[id] = role;
        (role == SpeakerRole::Interviewer ? interviewerTalk : subjectTalk) += stats[id].talkMs;
    }

    log_event("[SpeakerAssigner][assignSpeakers] Clusters relevantes: " + std::to_string(relevant.size()) +
              ", segmentos divididos: " + std::to_string(splitCount) +
              ". Investigador = cluster " + std::to_string(interviewer) + " (" + reason + "; preguntas " +
              formatScore(interviewerScore) + " vs " + formatScore(subjectScore) + "; habla total " +
              std::to_string(interviewerTalk / 1000) + "s vs " + std::to_string(subjectTalk / 1000) + "s)");

    // Fragmentos de clusters no relevantes (o sin cluster): heredan el rol del
    // fragmento anterior; un fragmento suelto en medio de un turno es casi
    // siempre la misma persona que venia hablando.
    std::vector<std::optional<SpeakerRole>> roles(pieces.size());
    for (size_t i = 0; i < pieces.size(); ++i) {
        if (!pieces[i].speakerId) continue;
        const auto it = roleOf.find(*pieces[i].speakerId);
        if (it != roleOf.end()) roles[i] = it->second;
    }
    std::optional<SpeakerRole> previous;
    for (size_t i = 0; i < roles.size(); ++i) {
        if (roles[i]) {
            previous = roles[i];
        } else if (previous) {
            roles[i] = previous;
        }
    }
    for (size_t i = roles.size(); i-- > 0;) {  // los del principio: el siguiente
        if (roles[i]) {
            previous = roles[i];
        } else {
            roles[i] = previous;
        }
    }

    for (size_t i = 0; i < pieces.size(); ++i) {
        result.push_back({pieces[i].startMs, pieces[i].endMs, roles[i], std::move(pieces[i].text)});
    }
    return result;
}

}  // namespace hermes::diarization
