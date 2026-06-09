#include "market.hpp"
#include "nlohmann/json_fwd.hpp"

#include <cpr/cpr.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;


static const std::string TV_ORIGIN = "https://www.tradingview.com";
static const std::string TV_HOME = "https://www.tradingview.com/";
static const std::string TV_LOGIN = "https://www.tradingview.com/accounts/signin/";
static const std::string TV_UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                                 "(KHTML, like Gecko) Chrome/147.0.7727.56 Safari/537.36";

namespace market {

// anon login - same data, same resolution unless premium account
TradingView::TradingView() {
    init_session();
}

TradingView::TradingView(std::string username, std::string password) {
    init_session();
    login(username, password);
}

// check TradingView accessible
void TradingView::init_session() {

    session.SetUserAgent(TV_UA);
    session.SetHeader(cpr::Header{
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"Accept-Language", "en-US,en;q=0.9"},
        {"Accept-Encoding", "gzip, deflate, br"},
        {"Origin", TV_ORIGIN},
        {"Referer", TV_HOME},
    });
    session.SetUrl(cpr::Url{TV_HOME});
    cpr::Response r = session.Get();

    std::cout << "[TV] init GET " << TV_HOME << " -> " << r.status_code << "\n";
    for (const auto& c : r.cookies) {
        std::cout << "  cookie: " << c.GetName() << "=" << c.GetValue() << "\n";
    }
    if (r.status_code == 200)
        init = true;
}

// Authenticate to tradingview | OPTIONIAL (!) unless premuim account
void TradingView::login(std::string username, std::string password) {
    if (init) {
        session.SetUrl(cpr::Url{TV_LOGIN});
    }
}

// anon mode for now; authenticated-session option (sessionid cookie) still TODO.
// One connection streams every symbol; Feed picks chart-series vs. quote frames.
void TradingView::subscribe(const std::vector<std::string>& symbols, Feed type,
                            intervals resolution) {
    ix::initNetSystem();

    ix::WebSocket ws;
    ws.setUrl("wss://data.tradingview.com/socket.io/websocket");
    ix::WebSocketHttpHeaders headers;
    headers["Origin"] = TV_ORIGIN;
    ws.setExtraHeaders(headers);

    auto rit = TV_INTERVAL_MAP.find(resolution);
    const std::string timeframe = (rit != TV_INTERVAL_MAP.end()) ? rit->second : "1";
    const int barCount = 300;

    std::unordered_map<std::string, std::string> labels; // sds_N -> symbol, for bar output
    std::unordered_map<std::string, std::map<long, double>> closes; // sds_N -> epoch -> close

    // bootstrap frames for the chosen feed, sent once the server session is ready

    /*
    order of operations:
        set authentication token
        inititate handshake with sessionID based on session type: series or quote
        creates respective session
        sets respective fields
        adds relevent symbols
    */

    auto handshake = [&]() {
        // temp, need to add option for authed user
        ws.send(Frame::command("set_auth_token", {"unauthorized_user_token"}));

        if (type == Feed::Quote) {
            const std::string qs = genSession("qs_");
            std::cout << "[handshake] session=" << qs << "\n";
            ws.send(Frame::command("quote_create_session", {qs}));
            ws.send(Frame::command("quote_set_fields",
                                   {qs, "lp", "ch", "chp", "volume", "bid", "ask"}));
            // quote_add_symbols: session id followed by every symbol, one frame
            json add = json::array({qs});
            for (const std::string& sym : symbols)
                add.push_back(sym);
            ws.send(Frame::command("quote_add_symbols", add));
            return;
        }

        // Feed::Series — anon tier allows one series per chart session, so open a
        // fresh chart session per symbol over the same connection.
        for (size_t i = 0; i < symbols.size(); ++i) {
            const std::string cs = genSession("cs_");
            const std::string symId = "sds_sym_" + std::to_string(i);
            const std::string seriesId = "sds_" + std::to_string(i);
            // session:"extended" -> include pre/post-market subsessions
            json spec = {{"symbol", symbols[i]}, {"adjustment", "splits"}, {"session", "extended"}};
            ws.send(Frame::command("chart_create_session", {cs, ""}));
            ws.send(Frame::command("resolve_symbol", {cs, symId, "=" + spec.dump()}));
            ws.send(Frame::command("create_series", {cs, seriesId, "s" + std::to_string(i), symId,
                                                     timeframe, barCount, ""}));
            labels[seriesId] = symbols[i];
        }
    };

    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
        case ix::WebSocketMessageType::Open:
            std::cout << "[open] connected\n";
            break;

        case ix::WebSocketMessageType::Message:
            for (const std::string& payload : Frame::parse(msg->str)) {
                // echo heartbeat to maintain connection
                if (Frame::isHeartbeat(payload)) { 
                    ws.send(Frame::wrap(payload));
                    continue;
                }
                
                json j = json::parse(payload, nullptr, false);
                if (j.is_discarded())
                    continue;

                // first server frame
                if (j.contains("session_id")) { 
                    handshake();
                    continue;
                }
                if (!j.contains("m"))
                    continue;
                const std::string m = j["m"].get<std::string>();
                
                // message type and printing
                if ((m == "timescale_update" || m == "du") && j["p"].size() > 1) {
                    printSeries(j["p"][1], labels, closes); // p[1] = { "sds_<i>": { "s": [bars] } }
                } else if (m == "qsd") {
                    printQuote(j["p"]);
                } else if (m == "symbol_error" || m == "series_error" || m == "critical_error" ||
                           m == "protocol_error") {
                    std::cout << "[" << m << "] " << j["p"].dump() << "\n";
                }
            }
            break;

        case ix::WebSocketMessageType::Error:
            std::cout << "[error] " << msg->errorInfo.reason << "\n";
            break;
        case ix::WebSocketMessageType::Close:
            std::cout << "[close] " << msg->closeInfo.code << " " << msg->closeInfo.reason << "\n";
            break;
        default:
            break;
        }
    });

    ws.start();
    std::cout << "Streaming " << (type == Feed::Quote ? "quotes" : "series") << " for "
              << symbols.size() << " symbol(s) - Enter to quit." << "\n";
    std::cin.get();

    ws.stop();
    ix::uninitNetSystem();
}

// randomly generate session id
std::string TradingView::genSession(const std::string& prefix) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, (int)sizeof(charset) - 2);
    std::string s = prefix;
    for (int i = 0; i < 12; ++i)
        s += charset[dist(gen)];
    return s;
}

// Wilder/RMA RSI, matching TradingView's ta.rsi. NaN until we have more than `period` closes.
double TradingView::computeRSI(const std::vector<double>& closes, int period) {
    if ((int)closes.size() <= period)
        return std::nan("");

    double gain = 0.0, loss = 0.0;
    for (int i = 1; i <= period; ++i) {
        double d = closes[i] - closes[i - 1];
        (d >= 0 ? gain : loss) += std::fabs(d);
    }
    gain /= period;
    loss /= period;
    for (size_t i = period + 1; i < closes.size(); ++i) {
        double d = closes[i] - closes[i - 1];
        gain = (gain * (period - 1) + (d > 0 ? d : 0.0)) / period;
        loss = (loss * (period - 1) + (d < 0 ? -d : 0.0)) / period;
    }
    if (loss == 0.0)
        return 100.0;
    return 100.0 - 100.0 / (1.0 + gain / loss);
}


std::string TradingView::fmtTime(long epoch) {
    std::time_t t = (std::time_t)epoch;
    char buf[32];
    std::tm tm;
    gmtime_s(&tm, &t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

void TradingView::printSeries(const json& seriesMap,
                              const std::unordered_map<std::string, std::string>& labels,
                              std::unordered_map<std::string, std::map<long, double>>& closes) {
    if (!seriesMap.is_object())
        return;

    // iterate every series id present so multi-symbol updates all print
    for (const auto& [seriesId, series] : seriesMap.items()) {
        if (!series.contains("s") || !series["s"].is_array())
            continue;

        auto it = labels.find(seriesId);
        const std::string label = (it != labels.end()) ? it->second : seriesId;

        auto& closeByTs = closes[seriesId]; // ordered map dedupes/sorts across full + delta frames
        double lastVol = std::nan("");
        for (const auto& bar : series["s"]) {
            if (!bar.contains("v") || !bar["v"].is_array() || bar["v"].size() < 6)
                continue;
            const auto& v = bar["v"]; // [time, open, high, low, close, volume]
            closeByTs[(long)v[0].get<double>()] = v[4].get<double>();
            lastVol = v[5].get<double>();
            std::cout << label << "  " << fmtTime((long)v[0].get<double>()) << " UTC  O=" << v[1]
                      << " H=" << v[2] << " L=" << v[3] << " C=" << v[4] << " V=" << v[5] << "\n";
        }

        std::vector<double> ordered;
        ordered.reserve(closeByTs.size());
        for (const auto& [ts, c] : closeByTs)
            ordered.push_back(c);

        double rsi = computeRSI(ordered);
        if (!std::isnan(rsi))
            std::cout << label << "  RSI(14)=" << rsi << "\n";
        if (!std::isnan(lastVol))
            std::cout << label << "  VOL=" << lastVol << "\n";
    }
}

// qsd payload: [ session, { "n":<symbol>, "s":"ok", "v":{ fields } } ]. Updates may
// be partial, so every field is printed only when present.
void TradingView::printQuote(const json& payload) {
    if (!payload.is_array() || payload.size() < 2)
        return;
    const auto& data = payload[1];
    if (!data.is_object() || !data.contains("n") || !data.contains("v"))
        return;

    const auto& v = data["v"];
    std::cout << data["n"].get<std::string>();
    if (v.contains("lp"))
        std::cout << "  last=" << v["lp"];
    if (v.contains("ch"))
        std::cout << " chg=" << v["ch"];
    if (v.contains("chp"))
        std::cout << " chg%=" << v["chp"];
    if (v.contains("volume"))
        std::cout << " vol=" << v["volume"];
    if (v.contains("bid"))
        std::cout << " bid=" << v["bid"];
    if (v.contains("ask"))
        std::cout << " ask=" << v["ask"];
    std::cout << "\n";
}


// IPC functionality for scheduler compatibility
// Idea:  WS runs on its own thread; Scheduler polls a thread-safe quote cache.

} // namespace market
