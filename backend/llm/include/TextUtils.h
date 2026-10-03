#pragma once

#include <cstddef>
#include <string>

// Utilidades de texto compartidas por TranscriptEnhancer y GlossarySanitizer.
// Operan sobre bytes: los caracteres UTF-8 no ASCII (acentos, ñ) quedan tal
// cual, lo cual alcanza para comparar variantes de una misma palabra.
namespace hermes::llm::text {

std::string trim(const std::string& text);

// Minusculas solo para ASCII; los bytes no ASCII no se tocan.
std::string toLower(std::string text);

// Distancia de edicion (insercion, borrado, sustitucion) entre dos strings.
size_t levenshtein(const std::string& a, const std::string& b);

}  // namespace hermes::llm::text
