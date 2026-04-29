#include "news.h"
#include "utils/utils.h"
#include <cpr/cpr.h>
#include <string>

static const std::string F_YAHOO = "https://feeds.finance.yahoo.com/rss/2.0/headline";
static const std::string YAHOO_RSS_NEWS = "https://finance.yahoo.com/news/rssindex";

namespace news::yahoo {

    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols) {
        cpr::Parameters params = cpr::Parameters{{"s", misc::join(symbols, ',')}, {"region", "US"}, {"lang", "en-US"}};
        return cpr::Get(cpr::Url{F_YAHOO}, headers, params);
    }

    cpr::Response rss_market_news() { return cpr::Get(cpr::Url{YAHOO_RSS_NEWS}, headers); }
} // namespace news