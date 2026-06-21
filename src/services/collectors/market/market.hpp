#pragma once

#include <cpr/cpr.h>
#include "nlohmann/json_fwd.hpp"

#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>


using json = nlohmann::json;

namespace market {

// d -> days
// m -> minutes
enum Intervals { _1m, _2m, _5m, _15m, _30m, _60m, _90m, _1d, _5d, _1wk, _1mo, _3mo };

extern const std::unordered_map<Intervals, std::string> YAHOO_INTERVAL_MAP;
extern const std::unordered_map<Intervals, std::string> TV_INTERVAL_MAP;


/// query yahoo api interface
class Yahoo {
public:
    Yahoo();
    /// get history endpoint for each ticker at period range
    cpr::Response get_hist(const std::string& ticker,
                           const time_t& period1,
                           const time_t& period2,
                           const market::Intervals& interval);
    /// get current symbols quotes
    cpr::Response get_quote(const std::vector<std::string>& symbols);
    /// get news for specific tickers
    cpr::Response get_ticker_news(const std::vector<std::string>& symbols, int news_count);

private:
    cpr::Session session;
    std::string host;
    std::string crumb;
    /// gets crumbs for authentication
    std::string get_crumb();
    /// general session based get request for authenticated market calls
    cpr::Response get(const std::string& url,
                      cpr::Parameters params = cpr::Parameters{},
                      cpr::Header headers = cpr::Header{});

    // util
    static std::string test_urls();
};

class TradingView {
public:
    /// which TradingView feed to open over the shared socket.io connection
    enum class Feed { Series, Quote };

    TradingView();
    TradingView(std::string username, std::string password);

    /// open a WSS connection and stream the requested feed for every symbol;
    /// `resolution` sets the bar timeframe for Series feeds (ignored for Quote)
    void subscribe(const std::vector<std::string>& symbols, Feed type = Feed::Series,
                   Intervals resolution = _1m);

private:
    bool init = false;
    cpr::Session session;
    void init_session();
    void login(std::string username, std::string password);

    /// socket.io v0.9 transport framing: ~m~<len>~m~<payload>
    class Frame {
    public:
        /// split a raw socket.io message into its individual payloads
        static std::vector<std::string> parse(const std::string& data);
        /// wrap a payload in a single ~m~<len>~m~ frame
        static std::string wrap(const std::string& payload);
        /// build and wrap a {"m":method,"p":params} command frame
        static std::string command(const std::string& method, const json& params);
        /// true if payload is a server heartbeat (~h~<n>)
        static bool is_heartbeat(const std::string& payload);
    };

    static std::string gen_session(const std::string& prefix);
    static std::string fmt_time(long epoch);
    /// print OHLCV bars from a timescale_update/du series map (labels: sds_N -> symbol),
    /// merge closes into `closes` (series_id -> epoch -> close) and print RSI + volume per series
    static void print_series(const json& series_map,
                             const std::unordered_map<std::string, std::string>& labels,
                             std::unordered_map<std::string, std::map<long, double>>& closes);
    /// Wilder/RMA RSI over an ordered close series; NaN until size > period
    static double compute_rsi(const std::vector<double>& closes, int period = 14);
    /// print a quote update from a qsd frame
    static void print_quote(const json& payload);
};

namespace cnn {

enum class FngTimescale { Now, Snapshot, HistoricalSeries };

// previous close
cpr::Response get_snapshot(FngTimescale timescale = FngTimescale::Now);

// fear_and_greed_historical.data: [{x: epoch_ms, y: score, rating}]
// from_epoch (unix seconds) trims how far back the series goes; 0 = full history
cpr::Response get_historical(time_t from_epoch = 0);

} // namespace cnn

}; // namespace market
