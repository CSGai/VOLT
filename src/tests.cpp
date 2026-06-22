#include "tests.hpp"

#include "services/collectors/market/market.hpp"
#include "services/collectors/news/news.hpp"

#include <cpr/cpr.h>

#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

static const cpr::Header RSS_HEADERS = {
    {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"},
};

static const std::string FRED_KEY = "";

static std::string fmt_time(time_t t) {
    char buf[32];
    std::tm tm_utc{};
    gmtime_s(&tm_utc, &t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M UTC", &tm_utc);
    return buf;
}

static void print_articles(const std::string& label, const std::vector<news::Article>& articles) {
    std::cout << "\n-- " << label << " (" << articles.size() << " articles) --\n";
    for (const auto& a : articles) {
        std::cout << "  [" << fmt_time(a.published_at) << "] " << a.title << "\n"
                   << "    " << a.publisher << " - " << a.url << "\n";
    }
}

static void fetch_and_print_rss(const std::string& label, const cpr::Url& url) {
    cpr::Response res = cpr::Get(url, RSS_HEADERS);
    if (res.status_code != 200) {
        std::cout << "\n-- " << label << " --\n  [ERROR] status " << res.status_code << "\n";
        return;
    }
    news::Rss rss;
    print_articles(label, rss.parse_rss(res));
}

void run_tests() {
    std::cout << "=== TradingBot tests ===\n";

    // // market::cnn - fear & greed: last close, 1 week, 1 month
    // std::cout << "\n-- market::cnn fear & greed --\n";
    // {
    //     cpr::Response res = market::cnn::get_snapshot(market::cnn::FngTimescale::Snapshot);
    //     if (res.status_code != 200) {
    //         std::cout << "  [ERROR] status " << res.status_code << "\n";
    //     } else {
    //         auto body = nlohmann::json::parse(res.text, nullptr, false);
    //         if (body.is_discarded()) {
    //             std::cout << "  [ERROR] malformed body\n";
    //         } else {
    //             std::cout << "  now:            " << body.value("score", -1.0) << " (" << body.value("rating", "?") << ")\n"
    //                       << "  previous close: " << body.value("previous_close", -1.0) << "\n"
    //                       << "  previous 1wk:   " << body.value("previous_1_week", -1.0) << "\n"
    //                       << "  previous 1mo:   " << body.value("previous_1_month", -1.0) << "\n";
    //         }
    //     }
    // }

    // // market::Yahoo - quotes for VIX & SPY
    // std::cout << "\n-- market::Yahoo quotes (^VIX, SPY) --\n";
    // {
    //     market::Yahoo yahoo;
    //     cpr::Response res = yahoo.get_quote({"^VIX", "SPY"});
    //     if (res.status_code != 200) {
    //         std::cout << "  [ERROR] status " << res.status_code << "\n";
    //     } else {
    //         auto body = nlohmann::json::parse(res.text, nullptr, false);
    //         if (body.is_discarded()) {
    //             std::cout << "  [ERROR] malformed body\n";
    //         } else {
    //             for (const auto& q : body.value("quoteResponse", nlohmann::json{}).value("result", nlohmann::json::array())) {
    //                 std::cout << "  " << q.value("symbol", "?") << " (" << q.value("shortName", "?") << "): "
    //                           << q.value("regularMarketPrice", 0.0) << " (" << q.value("regularMarketChangePercent", 0.0) << "%)\n";
    //             }
    //         }
    //     }
    // }

    // // news::yahoo - RSS headlines for VIX & SPY
    // {
    //     cpr::Response res = news::yahoo::get_ticker_headlines({"^VIX", "SPY"});
    //     if (res.status_code != 200) {
    //         std::cout << "\n-- news::yahoo ticker headlines (^VIX, SPY) --\n  [ERROR] status " << res.status_code << "\n";
    //     } else {
    //         news::Rss rss;
    //         print_articles("news::yahoo ticker headlines (^VIX, SPY)", rss.parse_rss(res));
    //     }
    // }

    // // news::yahoo - general market news RSS
    // {
    //     cpr::Response res = news::yahoo::rss_market_news();
    //     if (res.status_code != 200) {
    //         std::cout << "\n-- news::yahoo market news --\n  [ERROR] status " << res.status_code << "\n";
    //     } else {
    //         news::Rss rss;
    //         print_articles("news::yahoo market news", rss.parse_rss(res));
    //     }
    // }

    // // news::fed - upcoming economic event calendar
    // std::cout << "\n-- news::fed event calendar --\n";
    // {
    //     auto events = news::fed::get_event_calendar(FRED_KEY);
    //     if (events.empty()) {
    //         std::cout << "  [ERROR] no events returned\n";
    //     } else {
    //         for (const auto& e : events)
    //             std::cout << "  " << e.date << "  " << e.label << " (in " << e.days_until << " days)\n";
    //     }
    // }

    // // Dow Jones - breaking market news RSS
    // fetch_and_print_rss("Dow Jones breaking news", cpr::Url{"https://feeds.content.dowjones.io/public/rss/mw_bulletins"});

    // // CNBC - global news RSS
    // fetch_and_print_rss("CNBC global news",
    //                      cpr::Url{"https://search.cnbc.com/rs/search/combinedcms/view.xml?partnerId=wrss01&id=100727362"});
    
    market::TradingView tv{};
    tv.subscribe({"CBOE:VIX"}, market::TradingView::Feed::Series, market::Intervals::_5m);

    std::cout << "\n=== done ===\n";
}
