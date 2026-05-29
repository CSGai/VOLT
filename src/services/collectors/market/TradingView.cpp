#include "market.hpp"
#include "nlohmann/json_fwd.hpp"

#include <cpr/cpr.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

using json = nlohmann::json;
/*
relevant links:

WSS:
1. Market data feed -
   wss://data.tradingview.com/socket.io/websocket?from=chart%2F3GHJT0rl%2F&date=2026-05-08T11%3A50%3A33&type=chart&auth=sessionid
   The main streaming endpoint. Carries quote/trade/series ticks and chart
   study updates after subscribing with quote_add_symbols / resolve_symbol /
   create_series frames.
     - from=chart/<layoutId>/  : referring page (a saved chart layout); used
                                 server-side for routing/limits and to make
                                 the connection look like it came from the
                                 chart UI.
     - date=<ISO8601 UTC>      : client wall-clock at handshake; acts as a
                                 cache-buster / anti-replay hint.
     - type=chart              : connection profile (chart vs. screener vs.
                                 widget) - affects which symbol scopes the
                                 server lets you subscribe to.
     - auth=sessionid          : value of the `sessionid` cookie from a
                                 logged-in tradingview.com session. Empty
                                 (or the literal "sessionid") = anonymous
                                 tier with delayed data and tighter rate
                                 limits.

2. Private push feed -
   wss://pushstream.tradingview.com/message-pipe-ws/private_feed
   Per-user push channel (NOT market data). Server pushes events tied to
   your account: alert firings, idea/comment notifications, watchlist
   sync, private DMs. Requires a valid `sessionid` cookie on the upgrade
   request - without auth the server closes immediately.

3. Public push feed -
   wss://pushstream.tradingview.com/message-pipe-ws/public
   Broadcast channel any client can connect to anonymously. Used for
   site-wide announcements, public chat rooms, and global event fan-out.
   No auth needed.

HTTP:
   https://data.tradingview.com/socket.io/websocket?from=chart%2F&date=2026-05-08T11%3A50%3A33&type=chart&auth=sessionid
   Same path as (1) but over plain HTTPS. socket.io clients hit this first
   for the handshake / long-polling fallback before negotiating the
   `Upgrade: websocket` switch. We won't use it directly - IXWebSocket
   opens the wss:// upgrade in one shot - but it's handy for poking the
   endpoint with curl to inspect cookies and response headers without
   dealing with WS framing.

WSS (alternatives / siblings of (1)):
4. Premium data feed -
   wss://prodata.tradingview.com/socket.io/websocket
   Real-time, full-depth equivalent of (1). Requires a `sessionid` from a
   Pro / Pro+ / Premium account; the anonymous tier gets 401 on upgrade.

5. Widget data feed -
   wss://widgetdata.tradingview.com/socket.io/websocket
   Lower-privilege endpoint that the embeddable TradingView widgets talk
   to. Accepts anonymous connections more leniently than (1) and is a
   useful fallback when you don't have a `sessionid` yet but still want
   live (delayed) prices.

HTTP (auth / discovery / metadata):
6. Login -
   POST https://www.tradingview.com/accounts/signin/
   Form fields: `username`, `password`, `remember=on`. The 200 response
   body is JSON with `user.session_hash`; the Set-Cookie header is what
   actually matters - the `sessionid` cookie set here is the value you
   feed into the `auth=` query param of (1) and into the `Cookie:` header
   of the WSS upgrade. Needs the same Origin/UA/Referer headers we send
   in bootstrap_tv_session() to avoid the Cloudflare challenge.

7. Current user -
   GET https://www.tradingview.com/accounts/current_user/
   Returns the logged-in user profile JSON, or `{"username":"","..."}` if
   anonymous. Cheap way to verify a `sessionid` is still valid before
   opening the WSS.

8. Symbol search -
   GET
https://symbol-search.tradingview.com/symbol_search/?text=AAPL&type=stock&hl=1&exchange=&lang=en&domain=production
   Resolves a free-text query to canonical `EXCHANGE:TICKER` symbols
   (e.g. `NASDAQ:AAPL`) which is the form (1) requires in
   `quote_add_symbols` / `resolve_symbol` frames.

9. Scanner / screener -
   POST https://scanner.tradingview.com/<market>/scan
   JSON body with `filter`, `columns`, `sort`, `range`. Powers the stock
   screener; the cheapest way to pull a snapshot list (e.g. all S&P 500
   tickers with their last price / volume / change %) without opening a
   WSS at all. `<market>` is one of: america, crypto, forex, etc.

10. History (REST bars) -
    GET
https://history.tradingview.com/v1/history?symbol=NASDAQ:AAPL&resolution=1D&from=<unix>&to=<unix>
    Historical OHLCV bars over plain HTTPS. Equivalent to issuing a
    `create_series` frame on (1) but without holding a socket open -
    better for backfills.

11. News -
    GET
https://news-headlines.tradingview.com/v2/headlines?category=stock&client=web&lang=en&symbol=NASDAQ:AAPL
    GET https://news-headlines.tradingview.com/v2/story?id=<storyId>
    Per-symbol headline list and full-story fetch. No auth required.

12. Economic / earnings calendar -
    GET https://economic-calendar.tradingview.com/events?from=<ISO>&to=<ISO>&countries=US
    GET https://earnings-calendar.tradingview.com/earnings_calendar?from=<ISO>&to=<ISO>
    Macro events and upcoming earnings - useful inputs for scheduling
    around volatility windows.
*/

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
        {"Referer", TV_ORIGIN + "/"},
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

// CONFIRMED REAL TIME DATA from tradingview

// get sessionID
// Idea:  pull sessionid (+ sessionid_sign) from cookies; JWT lives separately.
// Watch: JWT expires ~24h; anon path uses "unauthorized_user_token".

// subscibe to WSS
// Idea:  open ix::WebSocket with Origin/UA/Cookie, then send the socket.io bootstrap frames.
// Watch: socket.io v0.9 framing (~m~<len>~m~...), not Engine.IO; auth frame must be first.

// process websocket data
// Idea:  parse frames, echo heartbeats on socket.io websocket, dispatch quote/bar updates to a
// cache. Watch: one read may hold many frames; missed heartbeat = silent drop; reconnect wipes
// session IDs.

// currently in anon mode. need to add authenticated session option and potential session cookies
// make version that works with multiple symbols
void TradingView::subscribe(std::string target) {
    ix::initNetSystem();

    ix::WebSocket ws;
    ws.setUrl("wss://data.tradingview.com/socket.io/websocket");
    ix::WebSocketHttpHeaders headers;
    headers["Origin"] = "https://data.tradingview.com";
    ws.setExtraHeaders(headers);

    const std::string chartSession = genSession("cs_");
    const std::string symbol = "NASDAQ:AAPL";
    const std::string timeframe =
        "1"; // 1-min; MUST be intraday for ext hrs. if premuim can go smaller.
    const int barCount = 300;

    std::atomic<bool> sent{false};

    auto handshake = [&]() {
        if (sent.exchange(true))
            return;
        std::cout << "[handshake] chart session=" << chartSession << "\n";

        // session:"extended" -> include pre/post-market subsessions.
        json spec = {{"symbol", symbol}, {"adjustment", "splits"}, {"session", "extended"}};
        std::string symbolArg = "=" + spec.dump();

        ws.send(command("set_auth_token", {"unauthorized_user_token"}));
        ws.send(command("chart_create_session", {chartSession, ""}));
        ws.send(command("resolve_symbol", {chartSession, "sds_sym_1", symbolArg}));
        ws.send(command("create_series",
                        {chartSession, "sds_1", "s1", "sds_sym_1", timeframe, barCount, ""}));
    };

    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
        case ix::WebSocketMessageType::Open:
            std::cout << "[open] connected\n";
            break;

        case ix::WebSocketMessageType::Message: {
            std::cout << "[raw] " << msg->str << "\n";

            for (const std::string& payload : parseFrames(msg->str)) {
                if (payload.rfind("~h~", 0) == 0) { // heartbeat -> echo
                    ws.send(frame(payload));
                    continue;
                }

                json j = json::parse(payload, nullptr, false);
                if (j.is_discarded())
                    continue;

                if (j.contains("session_id") && !sent) { // first server frame
                    handshake();
                    continue;
                }
                if (!j.contains("m"))
                    continue;
                const std::string m = j["m"].get<std::string>();

                if (m == "timescale_update" || m == "du") {
                    // p[1] holds the series map { "sds_1": { "s": [bars] } }
                    if (j["p"].size() > 1)
                        TradingView::printBars(j["p"][1]);
                } else if (m == "symbol_resolved") {
                    std::cout << "[resolved] " << symbol << "\n";
                } else if (m == "series_completed") {
                    std::cout << "[series_completed] initial load done\n";
                } else if (m == "symbol_error" || m == "series_error" || m == "critical_error" ||
                           m == "protocol_error") {
                    std::cout << "[" << m << "] " << j["p"].dump() << "\n";
                }
            }
            break;
        }
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
    std::cout << "Loading " << timeframe << "m extended-hours bars for " << symbol
              << " — Enter to quit.\n";
    std::cin.get();

    ws.stop();
    ix::uninitNetSystem();
}

// make all helper functions into a proper class later with proper parsing
std::vector<std::string> TradingView::parseFrames(const std::string& data) {
    std::vector<std::string> frames;
    size_t pos = 0;
    while (pos + 3 <= data.size() && data.compare(pos, 3, "~m~") == 0) {
        pos += 3;                           // skip leading ~m~
        size_t end = data.find("~m~", pos); // find length delimiter
        if (end == std::string::npos)
            break;
        int len = 0;
        try {
            len = std::stoi(data.substr(pos, end - pos));
        } catch (...) {
            break;
        }
        pos = end + 3; // skip to payload start
        if (pos + len > data.size())
            break;
        frames.push_back(data.substr(pos, len));
        pos += len;
    }
    return frames;
}

std::string TradingView::command(const std::string& method, const json& params) {
    json j;
    j["m"] = method;
    j["p"] = params;
    return frame(j.dump());
}

std::string TradingView::frame(const std::string& payload) {
    return "~m~" + std::to_string(payload.size()) + "~m~" + payload;
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

void TradingView::printBars(const json& payload) {
    if (!payload.is_object() || !payload.contains("sds_1"))
        return;
    const auto& series = payload["sds_1"];
    if (!series.contains("s") || !series["s"].is_array())
        return;

    for (const auto& bar : series["s"]) {
        if (!bar.contains("v") || !bar["v"].is_array() || bar["v"].size() < 6)
            continue;
        const auto& v = bar["v"]; // [time, open, high, low, close, volume]
        std::cout << fmtTime((long)v[0].get<double>()) << " UTC  O=" << v[1] << " H=" << v[2]
                  << " L=" << v[3] << " C=" << v[4] << " V=" << v[5] << "\n";
    }
}

std::string TradingView::fmtTime(long epoch) {
    std::time_t t = (std::time_t)epoch;
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::gmtime(&t));
    return buf;
}

// IPC functionality for scheduler compatibility
// Idea:  WS runs on its own thread; Scheduler polls a thread-safe quote cache.
// Watch: never block the WS thread; clarify if "IPC" means in-process or cross-process.

} // namespace market
