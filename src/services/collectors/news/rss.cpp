#include "utils/utils.h"
#include <cpr/cpr.h>
#include <string>

static const std::string FYAHOO_HEADLINES = "/rss/2.0/headline";
static const std::string YAHOO_RSS_NEWS = "https://finance.yahoo.com/news/rssindex";

static const std::string F_YAHOO = "https://feeds.finance.yahoo.com";

namespace news {
    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols) {
        cpr::Parameters params = cpr::Parameters{{"s", misc::join(symbols, ',')}, {"region", "US"}, {"lang", "en-US"}};
        return cpr::Get(F_YAHOO + FYAHOO_HEADLINES, params);
    }

    cpr::Response rss_yahoo_news() { return cpr::Get(YAHOO_RSS_NEWS); }
} // namespace news
