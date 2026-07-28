#include "../include/FfmpegAudioNormalizer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
}

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace hermes::audio {

namespace {

constexpr int TARGET_SAMPLE_RATE = 16000;
constexpr AVSampleFormat TARGET_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;

// RAII para los tipos en C de FFmpeg, mismo patron que MysqlDeleter/StmtDeleter
// en DatabaseManager.h: sin esto, cualquier salida temprana (excepcion) filtra
// el contexto de FFmpeg.
struct FormatCtxCloser {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) avformat_close_input(&ctx);
    }
};
struct CodecCtxDeleter {
    void operator()(AVCodecContext* ctx) const { avcodec_free_context(&ctx); }
};
struct SwrCtxDeleter {
    void operator()(SwrContext* ctx) const { swr_free(&ctx); }
};
struct PacketDeleter {
    void operator()(AVPacket* pkt) const { av_packet_free(&pkt); }
};
struct FrameDeleter {
    void operator()(AVFrame* frame) const { av_frame_free(&frame); }
};

std::string ffmpegError(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

// Escribe un encabezado WAV canonico (44 bytes) + PCM16 mono crudo. No se
// usa el muxer de avformat a proposito (ver FfmpegAudioNormalizer.h): esto
// evita depender de features de encoding que vcpkg.json no habilita.
void writeWavFile(const std::string& outputPath, const std::vector<int16_t>& samples) {
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo de salida: " + outputPath);
    }

    const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    const uint32_t byteRate = TARGET_SAMPLE_RATE * 1 /* mono */ * sizeof(int16_t);
    const uint16_t blockAlign = static_cast<uint16_t>(1 * sizeof(int16_t));
    const uint32_t riffSize = 36 + dataSize;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&riffSize), 4);
    out.write("WAVE", 4);

    out.write("fmt ", 4);
    const uint32_t fmtChunkSize = 16;
    const uint16_t audioFormat = 1;  // PCM
    const uint16_t numChannels = 1;
    const uint16_t bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    const uint32_t sampleRate = TARGET_SAMPLE_RATE;
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    if (!samples.empty()) {
        out.write(reinterpret_cast<const char*>(samples.data()), dataSize);
    }

    if (!out.good()) {
        throw std::runtime_error("Error escribiendo el WAV normalizado: " + outputPath);
    }
}

}  // namespace

void FfmpegAudioNormalizer::normalize(const std::string& inputPath, const std::string& outputPath) {
    AVFormatContext* rawFormatCtx = nullptr;
    if (avformat_open_input(&rawFormatCtx, inputPath.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("No se pudo abrir el audio de entrada: " + inputPath);
    }
    std::unique_ptr<AVFormatContext, FormatCtxCloser> formatCtx(rawFormatCtx);

    if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) {
        throw std::runtime_error("No se pudo leer la informacion de streams del audio: " + inputPath);
    }

    const AVCodec* decoder = nullptr;
    int streamIndex = av_find_best_stream(formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (streamIndex < 0 || decoder == nullptr) {
        throw std::runtime_error("El archivo no contiene un stream de audio decodificable: " + inputPath);
    }

    std::unique_ptr<AVCodecContext, CodecCtxDeleter> codecCtx(avcodec_alloc_context3(decoder));
    if (!codecCtx) {
        throw std::runtime_error("No se pudo alocar el contexto del decoder de audio");
    }
    if (avcodec_parameters_to_context(codecCtx.get(), formatCtx->streams[streamIndex]->codecpar) < 0) {
        throw std::runtime_error("No se pudieron copiar los parametros del codec de audio");
    }
    if (avcodec_open2(codecCtx.get(), decoder, nullptr) < 0) {
        throw std::runtime_error("No se pudo abrir el decoder de audio para: " + inputPath);
    }

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, 1);  // mono

    SwrContext* rawSwr = nullptr;
    int swrRc = swr_alloc_set_opts2(&rawSwr,
                                     &outLayout, TARGET_SAMPLE_FORMAT, TARGET_SAMPLE_RATE,
                                     &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
                                     0, nullptr);
    if (swrRc < 0 || rawSwr == nullptr) {
        av_channel_layout_uninit(&outLayout);
        throw std::runtime_error("No se pudo configurar el resampler de audio: " + ffmpegError(swrRc));
    }
    std::unique_ptr<SwrContext, SwrCtxDeleter> swrCtx(rawSwr);
    av_channel_layout_uninit(&outLayout);

    if (swr_init(swrCtx.get()) < 0) {
        throw std::runtime_error("No se pudo inicializar el resampler de audio");
    }

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (!packet || !frame) {
        throw std::runtime_error("No se pudo alocar buffers de decodificacion de audio");
    }

    std::vector<int16_t> pcmSamples;

    // swr_get_out_samples sobreestima a proposito (compensa el drift de
    // resampleo); solo copiamos las muestras que swr_convert realmente
    // devuelve.
    auto drainResampler = [&](const uint8_t** inData, int inSamples) {
        int outSamplesEstimate = static_cast<int>(swr_get_out_samples(swrCtx.get(), inSamples));
        if (outSamplesEstimate <= 0) {
            outSamplesEstimate = TARGET_SAMPLE_RATE;
        }
        std::vector<int16_t> outBuffer(outSamplesEstimate);
        uint8_t* outPtr = reinterpret_cast<uint8_t*>(outBuffer.data());

        int converted = swr_convert(swrCtx.get(), &outPtr, outSamplesEstimate, inData, inSamples);
        if (converted < 0) {
            throw std::runtime_error("Fallo al resamplear audio: " + ffmpegError(converted));
        }
        pcmSamples.insert(pcmSamples.end(), outBuffer.begin(), outBuffer.begin() + converted);
    };

    while (av_read_frame(formatCtx.get(), packet.get()) >= 0) {
        if (packet->stream_index == streamIndex) {
            if (avcodec_send_packet(codecCtx.get(), packet.get()) == 0) {
                while (avcodec_receive_frame(codecCtx.get(), frame.get()) == 0) {
                    drainResampler(const_cast<const uint8_t**>(frame->data), frame->nb_samples);
                    av_frame_unref(frame.get());
                }
            }
        }
        av_packet_unref(packet.get());
    }

    // Flush del decoder: puede quedar un frame pendiente sin un packet mas.
    avcodec_send_packet(codecCtx.get(), nullptr);
    while (avcodec_receive_frame(codecCtx.get(), frame.get()) == 0) {
        drainResampler(const_cast<const uint8_t**>(frame->data), frame->nb_samples);
        av_frame_unref(frame.get());
    }

    // Flush del resampler: puede quedar audio bufferizado internamente.
    drainResampler(nullptr, 0);

    if (pcmSamples.empty()) {
        throw std::runtime_error("El audio normalizado quedo vacio (0 muestras): " + inputPath);
    }

    writeWavFile(outputPath, pcmSamples);
}

}  // namespace hermes::audio
