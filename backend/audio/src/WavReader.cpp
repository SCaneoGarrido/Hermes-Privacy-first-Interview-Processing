#include "../include/WavReader.h"

#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace hermes::audio {

std::vector<float> readNormalizedWav(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("No se pudo abrir el audio normalizado: " + path);
    }

    char riffTag[4];
    char waveTag[4];
    in.read(riffTag, 4);
    in.seekg(4, std::ios::cur);  // tamaño total RIFF, no lo necesitamos
    in.read(waveTag, 4);
    if (std::string(riffTag, 4) != "RIFF" || std::string(waveTag, 4) != "WAVE") {
        throw std::runtime_error("El audio normalizado no es un WAV valido: " + path);
    }

    // Busca el chunk "data" en vez de asumir el offset fijo 44: es barato
    // y tolera que el header traiga chunks extra si el normalizador cambia.
    char chunkId[4];
    uint32_t chunkSize = 0;
    bool foundData = false;
    while (in.read(chunkId, 4)) {
        in.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (std::string(chunkId, 4) == "data") {
            foundData = true;
            break;
        }
        in.seekg(chunkSize, std::ios::cur);
    }

    if (!foundData) {
        throw std::runtime_error("El audio normalizado no tiene chunk 'data': " + path);
    }

    const size_t sampleCount = chunkSize / sizeof(int16_t);
    std::vector<int16_t> raw(sampleCount);
    in.read(reinterpret_cast<char*>(raw.data()), chunkSize);

    std::vector<float> samples(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        samples[i] = static_cast<float>(raw[i]) / 32768.0f;
    }
    return samples;
}

}  // namespace hermes::audio
