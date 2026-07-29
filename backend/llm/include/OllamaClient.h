#pragma once

#include "ILLMClient.h"

namespace hermes::llm {

// Implementacion sobre la API REST local de Ollama (POST /api/chat, sin
// streaming). Ver ADR-015 - cpp-httplib como Cliente HTTP para Ollama:
// trafico exclusivamente localhost, sin TLS.
//
// Sin estado propio mas alla de la config de conexion (a diferencia de
// WhisperTranscriber, no hay un contexto pesado que cargar) - no necesita
// mutex ni carga perezosa.
class OllamaClient : public ILLMClient {
    public:
        OllamaClient(std::string baseUrl, std::string model);

        std::string chat(const std::string& systemPrompt, const std::string& userPrompt) override;

    private:
        std::string m_baseUrl;
        std::string m_model;
};

}  // namespace hermes::llm
