#include "../include/TextUtils.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace hermes::llm::text {

std::string trim(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

size_t levenshtein(const std::string& a, const std::string& b) {
    std::vector<size_t> prev(b.size() + 1), curr(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (size_t i = 1; i <= a.size(); ++i) {
        curr[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            const size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

}  // namespace hermes::llm::text
