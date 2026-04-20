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
    // cin.get();
    return 0;
}

void tests() {

    time_t p1 = 1699549200;
    time_t p2 = 1731319200;

    // market tests
    cout << "Testing endpoints:" << endl;
    market::test_yahoo_endpoints("PLTR", p1, p2, market::_1d);
    cout << market::test_analysis() << endl;
    cout << "test finished" << endl << endl;

    // util tests
    cout << "testing utils:" << endl;
    tm dt_test;
    time_t p3;
    char buffer[20];

    gutils::dt::unix2datetime_gmt(&p1, &dt_test);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &dt_test);
    gutils::dt::datetime2unix(&dt_test, &p3);

    cout << "unix -> datetime: " << p1 << " -> " << buffer << endl;
    cout << "datetime -> unix: " << buffer << " -> " << p3 << endl;
    cout << "test finished" << endl << endl;

    
}
