#include "../include/OllamaClient.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace hermes::llm {

namespace {

// Ollama usa temperature ~0.8 por defecto si no se especifica - suficiente
// aleatoriedad para que el mismo audio produzca atribuciones de hablante
// distintas entre corridas (ver Ollama Integration Strategy en la vault,
// hallazgo del 2026-07-29: interview_id=1 vs interview_id=8, mismo audio,
// etiquetas distintas). temperature=0 + seed fijo no corrige que el modelo
// se equivoque, pero garantiza que se equivoque siempre igual.
constexpr double DETERMINISTIC_TEMPERATURE = 0.0;
constexpr int DETERMINISTIC_SEED = 42;

}  // namespace

OllamaClient::OllamaClient(std::string baseUrl, std::string model)
    : m_baseUrl(std::move(baseUrl)), m_model(std::move(model)) {}

std::string OllamaClient::chat(const std::string& systemPrompt, const std::string& userPrompt) {
    httplib::Client client(m_baseUrl);
    client.set_connection_timeout(5, 0);
    // Inferencia local en CPU puede tardar varios minutos con prompts
    // largos - timeout generoso en vez de fallar prematuramente.
    client.set_read_timeout(300, 0);

    nlohmann::json body;
    body["model"] = m_model;
    body["stream"] = false;
    body["options"] = {
        {"temperature", DETERMINISTIC_TEMPERATURE},
        {"seed", DETERMINISTIC_SEED},
    };
    body["messages"] = nlohmann::json::array({
        {{"role", "system"}, {"content", systemPrompt}},
        {{"role", "user"}, {"content", userPrompt}},
    });

    auto res = client.Post("/api/chat", body.dump(), "application/json");
    if (!res) {
        throw std::runtime_error(
            "No se pudo conectar con Ollama en " + m_baseUrl +
            " (" + httplib::to_string(res.error()) +
            "). ¿Esta corriendo? Ver 'Ollama Integration Strategy' en la vault.");
    }
    if (res->status != 200) {
        throw std::runtime_error("Ollama respondio " + std::to_string(res->status) + ": " + res->body);
    }

    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(res->body);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("Respuesta de Ollama no es JSON valido: ") + e.what());
    }

    if (!parsed.contains("message") || !parsed["message"].contains("content")) {
        throw std::runtime_error("Respuesta de Ollama con formato inesperado (falta message.content)");
    }

    return parsed["message"]["content"].get<std::string>();
}

}  // namespace hermes::llm
