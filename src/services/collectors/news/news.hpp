#pragma once
#include "utils/ipc.hpp"

#include <cpr/async.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace news {
struct Article {
    std::string title;
    std::string url;
    std::string content;
    std::string publisher;
    time_t published_at;
};

class Rss {
public:
    /// add rss links to subscriptions
    void subscribe(const std::vector<std::string>& rss_links);

    /// prints out current rss subscription refresh rates
    void print_refresh_rates();

    /// read-only access to current per-feed refresh-rate map
    const std::unordered_map<int, std::vector<std::string>>& get_refresh_rates() const { return refresh_rates; }

    /// sync wrapper around get_rss, exposed only so tests can verify the
    /// conditional-GET / 304 path; production code goes through scheduled_polling
    cpr::Response fetch_sync(cpr::Url link);

    /// runs get rss and outputs to cache based on interval
    /// pipeline 0x01 = training
    /// pipeline 1x02 = runtime inference
    void run(int interval, Destination dest = Destination::TRAINING);

private:
    struct CacheValidators {
        std::string etag;
        std::string last_modified;
    };

    std::vector<cpr::Url> rss_links;
    std::unordered_map<int, std::vector<std::string>> refresh_rates;
    std::unordered_map<std::string, CacheValidators> cache_validators;
    std::mutex cache_mutex;

    /// gets rss recommended refresh rate in minutes
    /// <ttl> tag if exists for every subscriptions. otherwise sets to a default
    void set_refresh_rates();

    /// get specific subscription rss (async; conditional GET via ETag/If-Modified-Since)
    cpr::AsyncWrapper<cpr::Response> get_rss(cpr::Url link);

    /// parse ttl tag and sets to value, if value outside of range 0-60, default
    int parse_ttl(const std::string& ttl_str);

    /// parse xml and extract wanted information into articles
    std::vector<news::Article> parse_rss(const cpr::Response& rss_res);

    /// crawl through at retrieve the actual article itself from link
    // std::string crawl_article(const std::string& link);
};
} // namespace news

namespace news::yahoo {
cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols);
cpr::Response rss_market_news();
} // namespace news::yahoo

namespace news::fed {

enum class Indicator { CPI, CoreCPI, PCE, CorePCE, GDP, Unemployment, FedFundsRate, PPI, Payrolls };

struct ApiKeys {
    std::string bls;  // api.bls.gov
    std::string bea;  // apps.bea.gov
    std::string fred; // api.stlouisfed.org  (FOMC calendar only)
};

struct Event {
    std::string label;
    std::string date;
    int days_until;
};

// next upcoming date per major market-moving release via FRED; res.text = [{event, date, days_until}] sorted ascending
std::vector<Event> get_event_calendar(const std::string& fred_key);

// latest `count` observations for one indicator (BLS/BEA/NY Fed routed by indicator)
std::optional<cpr::Response> get_indicator(Indicator ind, const ApiKeys& keys, int count = 3);

} // namespace news::fed