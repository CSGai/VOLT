#include "utils/utils.h"
#include <cpr/cpr.h>
#include <string>

static const std::string F_YAHOO = "https://feeds.finance.yahoo.com";
static const std::string FYAHOO_HEADLINES = "/rss/2.0/headline";
static const std::string YAHOO_RSS_NEWS = "https://finance.yahoo.com/news/rssindex";

static const cpr::Header headers = {
    {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"},
};

namespace news::rss {

    cpr::Response get_yahoo_ticker_headlines(const std::vector<std::string>& symbols) {
        cpr::Parameters params = cpr::Parameters{{"s", misc::join(symbols, ',')}, {"region", "US"}, {"lang", "en-US"}};
        return cpr::Get(cpr::Url{F_YAHOO + FYAHOO_HEADLINES}, headers, params);
    }

    cpr::Response rss_yahoo_market_news() { return cpr::Get(cpr::Url{YAHOO_RSS_NEWS}, headers); }
} // namespace news
