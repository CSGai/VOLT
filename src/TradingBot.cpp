// #include "TradingBot.h"
#include "services/collectors/market/market.h"
#include "services/collectors/news/news.h"
#include "utils/utils.h"

#include <ctime>
#include <iostream>
#include <ostream>

using namespace std;

void run_all_test();
void run_collectors_tests();
void run_rss_tests();

int main() {
    cout << "Begin Trading Bot." << endl;

    run_rss_tests();
    // run_services_tests();
    cin.get();
    return 0;
}

void run_all_test() { run_collectors_tests(); }

void run_collectors_tests() {
    cout << "--------services test--------" << endl << endl;
    time_t p1 = 1753035600;
    time_t p2 = 1773039200;

    api::Yahoo yahoo;
    const auto& hist = yahoo.get_hist("PLTR", p1, p2, api::_1d);
    const auto& quote = yahoo.get_quote({"PLTR", "AAPL", "GOOGL"});
    const auto& ticker_news = yahoo.get_ticker_news({"PLTR", "AAPL", "GOOGL"}, 10);
    const auto& headlines = news::yahoo::get_ticker_headlines({"PLTR", "AAPL", "GOOGL"});
    const auto& rss_news = news::yahoo::rss_market_news();

    cout << "Historical data test: " << hist.status_line << endl;
    cout << "Quote data test: " << quote.status_line << endl;
    cout << "Ticker news test: " << ticker_news.status_line << endl;
    cout << "headlines test: " << headlines.status_line << endl;
    cout << "rss_news test: " << rss_news.status_line << endl;

    json_utils::write_cache("data/history.json", json::parse(hist.text));
    json_utils::write_cache("data/quote.json", json::parse(quote.text));
    json_utils::write_cache("data/ticker_news.json", json::parse(ticker_news.text));
    cout << "writing data/headlines.xml to cache" << endl;
    xml_utils::parse(headlines.text).save_file("data/headlines.xml");
    cout << "writing data/rss_news.xml to cache" << endl;
    xml_utils::parse(rss_news.text).save_file("data/rss_news.xml");

    cout << "----test finished----" << endl << endl;
}

void run_rss_tests() {
    news::Rss rss_manager = news::Rss();
    rss_manager.subscribe(
        {"https://feeds.finance.yahoo.com/rss/2.0/headline?s=yhoo,goog&region=US&lang=en-US", "https://news.yahoo.com/rss/"});
    rss_manager.print_refresh_rates();
}