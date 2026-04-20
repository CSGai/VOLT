#include "TradingBot.h"
#include "APIs/market/market.h"
#include "utils/utils.h"

#include <ctime>
#include <iostream>
#include <ostream>

using namespace std;

void tests();

int main() {
  cout << "Begin Trading Bot." << endl;

  tests();
  cin.get();
  return 0;
}

void tests() {
  // market tests
  cout << "Testing endpoints:" << endl;
  time_t p1 = 1699549200;
  time_t p2 = 1731319200;
  market::test_endpoints("PLTR", p1, p2, market::_1d);
  cout << market::test_analysis() << endl;
  cout << "test finished" << endl;

  // util tests
  cout << "testing utils:" << endl;

  time_t unix_tstamp = 1699549200;
  char converted_tstamp[20];

  gutils::unix2datetime(converted_tstamp, sizeof(converted_tstamp),
                        unix_tstamp);
  cout << "unix tstamp: " << unix_tstamp
       << " converted to strftime: " << converted_tstamp << endl;
}
