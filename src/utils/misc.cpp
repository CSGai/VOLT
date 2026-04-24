#include <string>
#include <vector>

namespace misc {
    std::string join(const std::vector<std::string>& symbols, const char& delimiter) {
        // concat symbols with delimiter
        std::string str_symbols;
        for (size_t i = 0; i < symbols.size(); ++i) {
            if (i > 0) str_symbols += delimiter;
            str_symbols += symbols[i];
        }
        return str_symbols;
    }
} // namespace misc
