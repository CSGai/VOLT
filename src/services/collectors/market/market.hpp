#pragma once

#include <cpr/cpr.h>
#include "nlohmann/json_fwd.hpp"

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>


using json = nlohmann::json;

namespace market {

// d -> days
// m -> minutes
enum intervals { _1m, _2m, _5m, _15m, _30m, _60m, _90m, _1d, _5d, _1wk, _1mo, _3mo };

inline const std::unordered_map<market::intervals, std::string> INTERVAL_MAP = {
    {market::_1m, "1m"},
    {market::_2m, "2m"},
    {market::_5m, "5m"},
    {market::_15m, "15m"},
    {market::_30m, "30m"},
    {market::_60m, "60m"},
    {market::_90m, "90m"},
    {market::_1d, "1d"},
    {market::_5d, "5d"},
    {market::_1wk, "1wk"},
    {market::_1mo, "1mo"},
    {market::_3mo, "3mo"}};

/// query yahoo api interface
class Yahoo {
public:
    Yahoo();
    /// get history endpoint for each ticker at period range
    cpr::Response get_hist(const std::string& ticker,
                           const time_t& period1,
                           const time_t& period2,
                           const market::intervals& interval);
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
    cpr::Response Get(const std::string& url,
                      cpr::Parameters params = cpr::Parameters{},
                      cpr::Header headers = cpr::Header{});

    // util
    static std::string test_urls();
};

class TradingView {
public:
    TradingView();
    TradingView(std::string username, std::string password);
    void subscribe(std::string target);
private:
    boolean init = false;
    cpr::Session session;
    void init_session();
    void login(std::string username, std::string password);
    
    static std::vector<std::string> parseFrames(const std::string& data);
    static std::string genSession(const std::string& prefix);
    static std::string frame(const std::string& payload);
    static std::string command(const std::string& method, const json& params);
    static std::string fmtTime(long epoch);
    static void printBars(const json& payload);
};

}; // namespace market
