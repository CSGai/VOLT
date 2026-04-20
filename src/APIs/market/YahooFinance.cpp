#include "market.h"
#include <cpr/cpr.h>
#include <iostream>
#include <string>

namespace market {
// Make sure to add "User-Agent", "Mozilla/5.0" header otherwise yahoofinance
// rejects the request
void test_endpoints(std::string ticker, time_t period1, time_t period2,
                    market::intervals interval) {

  std::string data_interval = INTERVAL_MAP[interval];
  cpr::Header agent = cpr::Header{{"User-Agent", "Mozilla/5.0"}};

  cpr::Response r = cpr::Get(
      cpr::Url{YAHOO_ENDPOINT1 +
               "PLTR?period1=1699549200&period2=1731319200&interval=1d"},
      agent);

  if (r.status_code == 200) {
    std::cout << "Response body Q1: " << r.status_code << std::endl;
  } else {
    std::cout << "FAILED Q1" << std::endl;
    std::cout << r.status_code << std::endl;
  }

  cpr::Response r2 = cpr::Get(
      cpr::Url{YAHOO_ENDPOINT2 +
               "AAPL?period1=1699549200&period2=1731319200&interval=1d"},
      agent);
  if (r2.status_code == 200) {
    std::cout << "Response body Q2: " << r2.status_code << std::endl;
  } else {
    std::cout << "FAILED Q2" << std::endl;
    std::cout << r2.status_code << std::endl;
  }
}
} // namespace market
