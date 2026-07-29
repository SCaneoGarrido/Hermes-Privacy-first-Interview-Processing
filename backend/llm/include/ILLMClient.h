#pragma once

#include <string>

namespace hermes::llm {

// Oculta Ollama detras de esta interfaz (ADR-005, Filosofia de
// Repositorios). Mapea a POST /api/chat con "stream": false - una llamada
// bloqueante por invocacion, consistente con que el worker de Sprint 4 ya
// procesa un job a la vez de forma sincrona.
class ILLMClient {
    public:
        virtual ~ILLMClient() = default;

        // Envia un mensaje de sistema + uno de usuario, devuelve el texto
        // completo de la respuesta del modelo. Lanza std::runtime_error si
        // Ollama no responde, no tiene el modelo cargado, o la respuesta no
        // se puede parsear.
        virtual std::string chat(const std::string& systemPrompt, const std::string& userPrompt) = 0;
};

}  // namespace hermes::llm
