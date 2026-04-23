#pragma once
#include <ctime>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace dt {
    void unix2datetime_gmt(time_t* unix_tstamp, tm* dt);
    void unix2datetime_local(time_t* unix_tstamp, tm* dt);

    void datetime2unix(tm* dt, time_t* unix_tstamp);
} // namespace dt
namespace _json {
    void write_cache(const std::string& json_name, const json& json_data);
    json read_cache(const std::string& json_name);
    json jsonify(std::string data);
    json test_json();
} // namespace _json