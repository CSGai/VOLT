#pragma once
#include "cpr/cprtypes.h"
#include "cpr/session.h"
#include <cpr/response.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace news {
    struct Article {
            std::string title;
            std::string url;
            std::string content;
            std::string publisher;
            time_t published_at;
    };

    class Rss {
        public:
            Rss();
            /// add rss links to subscriptions
            void subscribe(const std::vector<std::string>& rss_links);

            /// prints out current rss subscription refresh rates
            void print_refresh_rates();

        private:
            cpr::Session session;
            std::vector<cpr::Url> rss_links;
            std::unordered_map<std::string, int> refresh_rates;

            /// gets rss recommended refresh rate in minutes
            /// <ttl> tag if exists for every subscriptions. otherwise sets to a default
            void set_refresh_rates();

            /// get specific subscription rss
            cpr::Response get_rss(cpr::Url link);

            /// parse ttl tag and sets to value, if value outside of range 0-60, default
            int parse_ttl(const std::string& ttl_str);

            /// parse xml and extract wanted information into articles
            std::vector<news::Article> parse_rss(const cpr::Response& rss_res);

            /// crawl through at retrieve the actual article itself from link
            std::string crawl_article(const std::string& link);
    };
} // namespace news

namespace news::yahoo {
    cpr::Response get_ticker_headlines(const std::vector<std::string>& symbols);
    cpr::Response rss_market_news();
} // namespace news::yahoo