// #include "TradingBot.h"
#include "services/collectors/market/market.h"
#include "utils/utils.h"

#include <ctime>
#include <iostream>
#include <ostream>

using namespace std;

void run_all_test();
void run_scrapping_tests();
void run_utils_tests();

int main() {
    cout << "Begin Trading Bot." << endl;

    run_scrapping_tests();
    // run_utils_tests();
    cin.get();
    return 0;
}

void run_all_test() {
    run_utils_tests();
    run_scrapping_tests();
}
// scrapping tests
void run_scrapping_tests() {

    cout << "--------market test--------" << endl << endl;
    time_t p1 = 1699549200;
    time_t p2 = 1731319200;

    api::Yahoo yahoo;
    const auto& hist = yahoo.get_hist("PLTR", p1, p2, api::_1d);
    const auto& quote = yahoo.get_quote({"PLTR", "AAPL", "GOOGL"});
    const auto& news = yahoo.get_news({"PLTR", "AAPL", "GOOGL"}, 10);
    const auto& headlines = yahoo.get_headlines({"PLTR", "AAPL", "GOOGL"});

    cout << "Historical data test: " << hist.status_line << endl;
    cout << "Quote data test: " << quote.status_line << endl;
    cout << "Ticker news test: " << news.status_line << endl;
    cout << "headlines test: " << headlines.status_line << endl;

    cout << "----test finished----" << endl << endl;
}
// utils tests
void run_utils_tests() {
    cout << "--------testing utils--------" << endl << endl;

    cout << "----datetime tests----" << endl;
    time_t p1 = 1699549200;
    tm dt_test;
    time_t p3;
    char buffer[20];
    dt::unix2datetime_gmt(&p1, &dt_test);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &dt_test);
    dt::datetime2unix(&dt_test, &p3);

    cout << "unix -> datetime: " << p1 << " -> " << buffer << endl;
    cout << "datetime -> unix: " << buffer << " -> " << p3 << endl;
    cout << "test finished" << endl << endl;

    cout << "----json tests----" << endl;
    static const auto& test_file = _json::test_json();
    _json::write_cache("cache/test.json", test_file);
    cout << "test cache: " <<_json::read_cache("cache/test.json") << std::endl;
}
