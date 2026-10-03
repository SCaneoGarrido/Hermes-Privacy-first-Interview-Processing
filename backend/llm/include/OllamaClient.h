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
        std::string chatStructured(const std::string& systemPrompt,
                                   const std::string& userPrompt,
                                   const std::string& jsonSchema,
                                   int maxTokens) override;

    private:
        std::string m_baseUrl;
        std::string m_model;

        // POST /api/chat comun a ambos metodos. jsonSchema vacio: respuesta
        // de texto libre.
        std::string sendChat(const std::string& systemPrompt,
                             const std::string& userPrompt,
                             const std::string& jsonSchema,
                             int maxTokens);
};

}  // namespace hermes::llm
