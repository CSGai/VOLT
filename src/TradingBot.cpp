#include "services/collectors/market/market.hpp"

#include <iostream>

using namespace std;

int main() {
    cout << "=== Trading Bot collector tests ===\n\n";
    market::TradingView tv;
    tv.subscribe({"NYSE:VRT", "NASDAQ:AVGO"}, market::TradingView::Feed::Series, market::_1m);
    cin.get();
    return 0;
}
