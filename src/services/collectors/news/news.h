#include <cpr/response.h>
#include <string>
#include <vector>


namespace news {
    struct Article;
}

namespace news::rss {
    cpr::Response get_yahoo_ticker_headlines(const std::vector<std::string>& symbols);
    cpr::Response rss_yahoo_market_news();
} // namespace news::yahoo