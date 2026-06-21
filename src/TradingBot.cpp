#include "services/collectors/market/market.hpp"

#include <iostream>

using namespace std;

int main() {
    cout << "=== Trading Bot collector tests ===\n\n";

    cout << "--- CNN Fear & Greed snapshot ---\n";
    cpr::Response snap = market::cnn::get_snapshot();
    cout << "status: " << snap.status_code << "\n" << snap.text << "\n\n";

    // cout << "--- CNN Fear & Greed historical ---\n";
    // cpr::Response hist = market::cnn::get_historical();
    // cout << "status: " << hist.status_code << "\n" << hist.text << "\n\n";
    cin.get();
    market::TradingView tv;
    tv.subscribe({"NYSE:VRT", "NASDAQ:AVGO"}, market::TradingView::Feed::Series, market::_1m);
    cin.get();
    return 0;
}
