#pragma once

#include "IDiarizer.h"
#include "../../transcript/include/TranscriptModel.h"
#include "../../transcription/include/ITranscriber.h"

#include <vector>

namespace hermes::diarization {

// Cruza la transcripcion de whisper con la diarizacion acustica y asigna un
// rol (investigador / sujeto) a cada fragmento. Pura: sin I/O (solo loguea
// los puntajes de la decision de roles).
//
// 1. Cada segmento va al hablante que cubre >= 70% de su duracion, si el otro
//    no ocupa 2s o mas.
// 2. Si cruza un cambio de hablante y tiene palabras con tiempo, se divide
//    por oracion: cada oracion va al hablante con mas tiempo en su intervalo.
//    Si el segmento es una sola oracion, palabra por palabra: cada palabra va
//    al hablante de su punto medio, tramos de menos de 2 palabras se absorben
//    en el vecino, y cada corte se mueve al fin de oracion mas cercano.
//    En habla superpuesta gana el tramo de diarizacion mas largo.
// 3. Rol: el cluster con mayor proporcion de preguntas ("¿"/"?") es el
//    investigador; si empatan (diferencia < 0.05), el que menos habla; si
//    siguen empatados, el que habla primero. El usuario corrige en el editor.
//
// Si la diarizacion encontro menos de 2 hablantes, devuelve los segmentos sin
// hablante (no se puede distinguir investigador de sujeto).
std::vector<hermes::transcript::TranscriptTurn> assignSpeakers(
    const std::vector<hermes::transcription::TranscriptSegment>& segments,
    const std::vector<SpeakerTurn>& speakerTurns);

}  // namespace hermes::diarization
