#pragma once
#include "cpr/session.h"
#include <cpr/response.h>
#include <string>
#include <vector>

namespace news {
    struct Article {
            std::string title;
            std::string url;
            std::string description;
            std::string content;
            std::string publisher;
            time_t published_at;
    };

    class Rss {
        public:
            Rss();
            /// add rss links to subscriptions
            void subscribe(const std::vector<std::string>& rss_links);

        private:
            int refresh_rate;
            cpr::Session session;
            std::vector<cpr::Url> rss_links;
            /// get all subscription responses at once
            std::vector<cpr::Response> get_all_rss();
            /// parse xml and extract wanted information into articles
            std::vector<news::Article> parse_rss(const cpr::Response& rss_res);
            /// crawl through at retrieve the actual article itself from link
            std::string crawl_article(std::string link);
    };
} // namespace news

namespace news::yahoo {
    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols);
    cpr::Response rss_market_news();
} // namespace news::yahoo