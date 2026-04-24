#pragma once
#include <ctime>
#include <filesystem>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace dt {
    void unix2datetime_gmt(time_t* unix_tstamp, tm* dt);
    void unix2datetime_local(time_t* unix_tstamp, tm* dt);

    void datetime2unix(tm* dt, time_t* unix_tstamp);
} // namespace dt
namespace _json {
    void write_cache(const fs::path& path, const json& data);
    json read_cache(const std::filesystem::path& path);
    json jsonify(std::string data);
    json test_json();
} // namespace _json
namespace misc {
    std::string join(const std::vector<std::string>& symbols, const char& delimiter);
}