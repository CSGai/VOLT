#include "market.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace market {

// ---- Frame: socket.io v0.9 transport framing (~m~<len>~m~<payload>) ----

std::vector<std::string> TradingView::Frame::parse(const std::string& data) {
    std::vector<std::string> frames;
    size_t pos = 0;
    while (pos + 3 <= data.size() && data.compare(pos, 3, "~m~") == 0) {
        // skip leading ~m~
        pos += 3;
        
        // find length delimiter
        size_t end = data.find("~m~", pos);
        
        if (end == std::string::npos)
            break;
        int len = 0;
        try {
            len = std::stoi(data.substr(pos, end - pos));
        } catch (...) {
            break;
        }
        // skip to payload start
        pos = end + 3;
        if (pos + len > data.size())
            break;
        frames.push_back(data.substr(pos, len));
        pos += len;
    }
    return frames;
}

std::string TradingView::Frame::command(const std::string& method, const json& params) {
    json j;
    j["m"] = method;
    j["p"] = params;
    return wrap(j.dump());
}

std::string TradingView::Frame::wrap(const std::string& payload) {
    return "~m~" + std::to_string(payload.size()) + "~m~" + payload;
}

bool TradingView::Frame::is_heartbeat(const std::string& payload) {
    return payload.rfind("~h~", 0) == 0;
}

} // namespace market
