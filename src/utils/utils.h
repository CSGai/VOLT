#pragma once
#include <ctime>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <pugixml.hpp>

using json = nlohmann::json;

namespace dt_utils {
    /// prase rfc822 and rfc3339 to a unix timestamp
    time_t parse_rfc(const std::string& dateStr);
} // namespace dt_utils
namespace json_utils {
    /// write json cache to path
    void write_cache(const std::filesystem::path& path, const json& data);
    /// read json cache at path
    json read_json(const std::filesystem::path& path);
    json test_json();
} // namespace json_utils
namespace xml_utils {
    /// pugi based parsing for string to xml
    pugi::xml_document parse(const std::string& text);
}
namespace misc {
    /// join string on delimiter
    std::string str_join(const std::vector<std::string>& symbols, const char& delimiter);
    /// remove substring from string
    std::string str_remove(std::string str, const std::string& sub);
}