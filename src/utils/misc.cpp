#include <string>
#include <vector>

namespace utils::misc {
    
std::string str_join(const std::vector<std::string>& symbols, const char& delimiter) {
    // concat symbols with delimiter
    std::string str_symbols;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0)
            str_symbols += delimiter;
        str_symbols += symbols[i];
    }
    return str_symbols;
}

std::string str_remove(std::string str, const std::string& sub) {
    size_t pos = str.find(sub);
    if (pos != std::string::npos)
        str.erase(pos, sub.length());
    return str;
}
} // namespace utils::misc