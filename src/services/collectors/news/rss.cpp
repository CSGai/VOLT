#include "news.h"
#include "utils/utils.h"
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int default_refresh_rate = 15;

namespace news {
    Rss::Rss() {
        this->session = cpr::Session();
        this->session.SetUserAgent("Mozilla/5.0");
        // add header for rss change check
    }

    void Rss::subscribe(const std::vector<std::string>& rss_links) {
        for (const std::string& link : rss_links) {
            this->rss_links.emplace_back(cpr::Url{link});
        }
        set_refresh_rates();
    }

    void Rss::print_refresh_rates() {
        for (const auto& [url, ttl] : this->refresh_rates) {
            std::cout << url << ": " << ttl << "\n";
        }
    }

    // iterate over rss subscriptions and sends get ttl respectively
    void Rss::set_refresh_rates() {
        std::unordered_map<std::string, int> result;
        for (const cpr::Url& link : this->rss_links) {
            session.SetUrl(link);
            cpr::Response res = session.Get();
            if (res.status_code == 200) {
                // check for <ttl> tag. if doesn't exist, default
                auto doc = xml_utils::parse(res.text);
                auto channel = doc.child("rss").child("channel");
                int ttl = parse_ttl(channel.child_value("ttl"));
                result[res.url.str()] = ttl;
            } else
                std::cerr << "failed to reach RSS: " << link.str();
        }
        this->refresh_rates = result;
    }

    // get specific subscription rss
    cpr::Response Rss::get_rss(cpr::Url link) {
        session.SetUrl(link);
        cpr::Response res = session.Get();
        if (res.status_code != 200) std::cerr << "failed to reach RSS: " << link.str();
        return res;
    }

    // parse ttl value
    int Rss::parse_ttl(const char* ttl_str) {
        if (!ttl_str || ttl_str[0] == '\0') return default_refresh_rate;
        int ttl = std::stoi(ttl_str);
        if (ttl <= 0) return default_refresh_rate;
        // cap at 1 hour
        if (ttl > 60) return 60;
        return ttl;
    }

    // parse xml and extract wanted information into articles
    std::vector<news::Article> Rss::parse_rss(const cpr::Response& rss_res) {

        std::string domain = misc::str_remove(rss_res.url.str(), "https://");
        domain = misc::str_remove(domain, "http://");
        domain = domain.substr(0, domain.find('/'));

        auto xml = xml_utils::parse(rss_res.text);

        // extract articles
        std::vector<news::Article> articles;
        auto channel = xml.child("rss").child("channel");
        for (auto item : channel.children("item")) {
            news::Article article;
            article.title = item.child_value("title");
            article.url = item.child_value("link");
            article.description = item.child_value("description");
            // article.content = crawl_article(item.child_value("link"));
            article.publisher = item.child("source") ? item.child_value("source") : domain;
            article.published_at = dt_utils::parse_rfc(item.child_value("pubDate"));
            articles.push_back(article);
        }

        return articles;
    }

    // crawl rss sublink
    // std::string Rss::crawl_article(std::string link) {}

}; // namespace news
