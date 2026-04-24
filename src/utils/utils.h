#pragma once
#include <ctime>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <pugixml.hpp>

using json = nlohmann::json;

namespace dt_utils {
    void unix2datetime_gmt(time_t* unix_tstamp, tm* dt);
    void unix2datetime_local(time_t* unix_tstamp, tm* dt);

    void datetime2unix(tm* dt, time_t* unix_tstamp);
} // namespace dt
namespace json_utils {
    void write_cache(const std::filesystem::path& path, const json& data);
    json read_json(const std::filesystem::path& path);
    json test_json();
} // namespace _json
namespace xml_utils {
    pugi::xml_document parse(const std::string& text);
}
namespace misc {
    std::string join(const std::vector<std::string>& symbols, const char& delimiter);
}