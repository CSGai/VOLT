#pragma once
#include <cpr/response.h>
#include <string>
#include <vector>

namespace news {
    struct Article;
    class Rss {
        public:
            Rss(const std::vector<std::string>& rss_links);

        private:
            std::vector<cpr::Url> rss_links;
    };
} // namespace news

namespace news::yahoo {
    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols);
    cpr::Response rss_market_news();
} // namespace news::yahoo

static const cpr::Header headers = {
    {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"},
};