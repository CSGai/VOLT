#include "market.h"

#include <cpr/cpr.h>
#include <format>
#include <iostream>
#include <string>

namespace market {
    // Make sure to add "User-Agent", "Mozilla/5.0" header otherwise yahoofinance
    // rejects the request
    void test_yahoo_endpoints(std::string ticker, time_t period1, time_t period2, market::intervals interval) {
        std::string endpoint1 = YAHOO_FIN + YAHOO_HIST_ENDPOINT;
        std::string endpoint2 = YAHOO_FIN2 + YAHOO_HIST_ENDPOINT;

        std::string params = std::format("{}?period1={}&period2={}&interval={}", ticker, period1, period2, INTERVAL_MAP[interval]);
        cpr::Header agent = cpr::Header{{"User-Agent", "Mozilla/5.0"}};
        
        // test endpoint Query1
        cpr::Response r = cpr::Get(cpr::Url{endpoint1 + params}, agent);
        if (r.status_code == 200) std::cout << "Success Q1: " << r.status_code << std::endl;
        else { std::cout << "FAILED Q1 " << r.status_code << std::endl; }
        // test endpoint Query2
        cpr::Response r2 = cpr::Get(cpr::Url{endpoint2 + params}, agent);
        if (r2.status_code == 200) std::cout << "Success Q2: " << r2.status_code << std::endl;
        else { std::cout << "FAILED Q2 " << r2.status_code << std::endl; };
    }
} // namespace market
