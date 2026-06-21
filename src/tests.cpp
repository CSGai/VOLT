#include "tests.hpp"

#include "services/collectors/market/market.hpp"
#include "services/collectors/news/news.hpp"

#include <cpr/cpr.h>

#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

static void pass(const std::string& label) {
    std::cout << "[PASS] " << label;
}

static void fail(const std::string& label, int status) {
    std::cout << "[FAIL] " << label << " (status " << status << ")\n";
}

static void check(const std::string& label, const std::optional<cpr::Response>& res) {
    if (res && res->status_code == 200) {
        pass(label);
    } else
        fail(label, res->status_code);
}

// for endpoints that embed errors in the body (BEA) or can return empty results (BLS without key)
static void check_array(const std::string& label, const std::optional<cpr::Response>& res) {
    if (res && res->status_code != 200) {
        fail(label, res->status_code);
        return;
    }
    auto j = nlohmann::json::parse(res->text, nullptr, false);
    if (j.is_discarded() || !j.is_array() || j.empty())
        std::cout << "[FAIL] " << label << " (empty or malformed body)\n";
    else
        pass(label);
}

static const news::fed::ApiKeys FED_KEYS{
    .bls = "OOPS",
    .bea = "OOPS",
    .fred = "OOPS",
};

void run_tests() {
    std::cout << "=== TradingBot tests ===\n";

    // news::fed
    std::cout << "\n-- news::fed --\n";
    for (const auto& s : news::fed::get_event_calendar(FED_KEYS.fred))
        std::cout << "event_calendar" << s.label << ", " << s.date << ", " << s.days_until << '\n';
    
    check_array("get_indicator/CPI", news::fed::get_indicator(news::fed::Indicator::CPI, FED_KEYS));
    check_array("get_indicator/GDP", news::fed::get_indicator(news::fed::Indicator::GDP, FED_KEYS));
    check("get_indicator/EFFR", news::fed::get_indicator(news::fed::Indicator::FedFundsRate, FED_KEYS));

    // news::yahoo
    std::cout << "\n-- news::yahoo --\n";
    check("rss_market_news", news::yahoo::rss_market_news());
    check("get_ticker_headlines", news::yahoo::get_ticker_headlines({"AAPL"}));

    // news::Rss
    std::cout << "\n-- news::Rss --\n";
    {
        news::Rss rss;
        check("Rss::fetch_sync",
              rss.fetch_sync(cpr::Url{"https://feeds.finance.yahoo.com/rss/2.0/headline?s=AAPL&region=US&lang=en-US"}));
    }

    // market::cnn
    std::cout << "\n-- market::cnn --\n";
    check("cnn::get_snapshot", market::cnn::get_snapshot());
    check("cnn::get_historical", market::cnn::get_historical(std::time(nullptr) - 60 * 60 * 24 * 30));

    // market::Yahoo
    std::cout << "\n-- market::Yahoo --\n";
    {
        market::Yahoo yahoo;
        std::time_t now = std::time(nullptr);
        std::time_t week_ago = now - 60 * 60 * 24 * 7;
        check("Yahoo::get_hist", yahoo.get_hist("AAPL", week_ago, now, market::_1d));
        check("Yahoo::get_quote", yahoo.get_quote({"AAPL", "MSFT"}));
        check("Yahoo::get_ticker_news", yahoo.get_ticker_news({"AAPL"}, 3));
    }

    std::cout << "\n=== done ===\n";
}
