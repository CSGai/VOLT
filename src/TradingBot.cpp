#include "services/collectors/market/market.h"
#include "services/collectors/news/news.h"
#include "utils/utils.h"

#include <ctime>
#include <iostream>
#include <string>

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, name)                                                                                                       \
    do {                                                                                                                              \
        ++tests_run;                                                                                                                  \
        if (cond) {                                                                                                                   \
            ++tests_passed;                                                                                                           \
            cout << "  [PASS] " << name << "\n";                                                                                      \
        } else {                                                                                                                      \
            cerr << "  [FAIL] " << name << "\n";                                                                                      \
        }                                                                                                                             \
    } while (0)

static bool contains(const string& body, const string& needle) { return body.find(needle) != string::npos; }

static void test_yahoo_market() {
    cout << "[yahoo market]\n";
    api::Yahoo yahoo;

    // historical bars
    time_t p1 = 1753035600;
    time_t p2 = 1773039200;
    auto hist = yahoo.get_hist("PLTR", p1, p2, api::_1d);
    TEST_ASSERT(hist.status_code == 200, "get_hist status 200");
    TEST_ASSERT(contains(hist.text, "\"chart\""), "get_hist body has chart key");
    TEST_ASSERT(contains(hist.text, "PLTR"), "get_hist body references PLTR");

    // realtime quotes
    auto quote = yahoo.get_quote({"PLTR", "AAPL", "GOOGL"});
    TEST_ASSERT(quote.status_code == 200, "get_quote status 200");
    TEST_ASSERT(contains(quote.text, "PLTR") && contains(quote.text, "AAPL") && contains(quote.text, "GOOGL"),
                "get_quote body contains all requested symbols");

    // per-ticker news
    auto news = yahoo.get_ticker_news({"PLTR", "AAPL", "GOOGL"}, 10);
    TEST_ASSERT(news.status_code == 200, "get_ticker_news status 200");
    TEST_ASSERT(contains(news.text, "\"news\""), "get_ticker_news body has news array");
}

static void test_yahoo_news() {
    cout << "[yahoo news]\n";

    auto headlines = news::yahoo::get_ticker_headlines({"PLTR", "AAPL", "GOOGL"});
    TEST_ASSERT(headlines.status_code == 200, "get_ticker_headlines status 200");
    TEST_ASSERT(contains(headlines.text, "<rss") || contains(headlines.text, "<item"), "get_ticker_headlines body is RSS");

    auto market_news = news::yahoo::rss_market_news();
    TEST_ASSERT(market_news.status_code == 200, "rss_market_news status 200");
    TEST_ASSERT(contains(market_news.text, "<rss") || contains(market_news.text, "<item"), "rss_market_news body is RSS");
}

static void test_rss_subscribe() {
    cout << "[rss subscribe]\n";
    const vector<string> feeds = {"https://feeds.finance.yahoo.com/rss/2.0/headline?s=yhoo,goog&region=US&lang=en-US",
                                  "https://news.yahoo.com/rss/"};

    news::Rss rss;
    rss.subscribe(feeds);

    const auto& rates = rss.get_refresh_rates();
    TEST_ASSERT(rates.size() == feeds.size(), "subscribe populated refresh_rates for every feed");

    bool all_in_range = !rates.empty();
    for (const auto& [url, ttl] : rates) {
        if (ttl < 1 || ttl > 60) all_in_range = false;
    }
    TEST_ASSERT(all_in_range, "all refresh_rates within [1, 60] minutes");
}

static void test_rss_conditional_get_304() {
    cout << "[rss conditional GET / 304]\n";
    const cpr::Url feed{"https://finance.yahoo.com/news/rssindex"};

    news::Rss rss;
    auto first = rss.fetch_sync(feed);
    TEST_ASSERT(first.status_code == 200, "first fetch returns 200");

    // second call — same feed within seconds. Server should respond 304 because
    // the Rss class now sends If-None-Match / If-Modified-Since from the cached validators.
    auto second = rss.fetch_sync(feed);
    TEST_ASSERT(second.status_code == 304, "second fetch returns 304 (conditional GET hit)");
}

int main() {
    cout << "=== Trading Bot collector tests ===\n\n";

    test_yahoo_market();
    test_yahoo_news();
    test_rss_subscribe();
    test_rss_conditional_get_304();

    cout << "\n=== Summary: " << tests_passed << "/" << tests_run << " passed ===\n";
    cin.get();
    return tests_passed == tests_run ? 0 : 1;
}
