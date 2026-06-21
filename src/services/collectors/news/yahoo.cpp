#include "news.hpp"
#include "utils/utils.hpp"

#include <cpr/cpr.h>

#include <string>

static const std::string F_YAHOO = "https://feeds.finance.yahoo.com/rss/2.0/headline";
static const std::string YAHOO_RSS_NEWS = "https://finance.yahoo.com/news/rssindex";
static const cpr::Header headers = {
    {"User-Agent",
     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
     "Chrome/124.0.0.0 Safari/537.36"},
};

namespace news::yahoo {

cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols) {
    cpr::Parameters params = cpr::Parameters{{"s", utils::misc::str_join(symbols, ',')}, {"region", "US"}, {"lang", "en-US"}};
    return cpr::Get(cpr::Url{F_YAHOO}, headers, params);
}

cpr::Response rss_market_news() {
    return cpr::Get(cpr::Url{YAHOO_RSS_NEWS}, headers);
}
} // namespace news::yahoo