#include "market.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static const std::string CNN_FNG_BASE =
    "https://production.dataviz.cnn.io/index/fearandgreed/graphdata";

static const cpr::Header CNN_HEADERS = {
    {"User-Agent",
     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
     "Chrome/124.0.0.0 Safari/537.36"},
    {"Referer", "https://www.cnn.com/markets/fear-and-greed"},
};

namespace market::cnn {

cpr::Response get_snapshot(FngTimescale timescale) {
    cpr::Response res = cpr::Get(cpr::Url{CNN_FNG_BASE}, CNN_HEADERS);
    if (res.status_code != 200)
        return res;

    auto body = json::parse(res.text, nullptr, false);
    if (body.is_discarded())
        return res;

    switch (timescale) {
    case FngTimescale::Now:
        // current score and rating only
        res.text = json{{"score",  body["fear_and_greed"]["score"]},
                        {"rating", body["fear_and_greed"]["rating"]}}.dump();
        break;
    case FngTimescale::Snapshot:
        // full object: score, rating, timestamp, previous_close, previous_1_week, previous_1_month, previous_1_year
        res.text = body["fear_and_greed"].dump();
        break;
    case FngTimescale::HistoricalSeries:
        // [{x: epoch_ms, y: score, rating}]
        res.text = body["fear_and_greed_historical"].dump();
        break;
    }
    return res;
}

// fear_and_greed_historical.data: [{x: epoch_ms, y: score, rating}]
// from_epoch (unix seconds) trims how far back the series goes; 0 = full history
cpr::Response get_historical(time_t from_epoch) {
    std::string url = CNN_FNG_BASE;
    if (from_epoch > 0)
        url += "/" + std::to_string(from_epoch);
    return cpr::Get(cpr::Url{url}, CNN_HEADERS);
}

} // namespace market::cnn
