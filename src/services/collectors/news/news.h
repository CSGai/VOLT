#include <string>
#include <vector>
#include <cpr/response.h>

namespace news {
    struct Article;

    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols);
    cpr::Response rss_yahoo_news();
}