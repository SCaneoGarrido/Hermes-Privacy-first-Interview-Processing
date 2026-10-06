#pragma once

#include <string>
#include <vector>

namespace hermes::audio {

// Lee el WAV canonico que escribe FfmpegAudioNormalizer (PCM 16-bit, 16kHz,
// mono) y devuelve las muestras normalizadas a [-1, 1]. No es un parser WAV
// general: si algun dia IAudioNormalizer cambia de formato de salida, esto
// tiene que cambiar junto con el. Lo comparten la transcripcion y la
// diarizacion, que leen el mismo audio.wav.
// Lanza std::runtime_error si el archivo no existe o no es un WAV valido.
std::vector<float> readNormalizedWav(const std::string& path);

}  // namespace hermes::audio
