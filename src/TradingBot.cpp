#include "services/collectors/market/market.hpp"

#include <iostream>

using namespace std;

int main() {
    cout << "=== Trading Bot collector tests ===\n\n";
    market::TradingView tv;
    tv.subscribe("NYSE:VRT");
    cin.get();
    return 0;
}
