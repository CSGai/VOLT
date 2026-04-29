#include "news.h"
#include "utils/utils.h"
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <vector>

constexpr int refresh_rate = 15;

namespace news {
    Rss::Rss() {
        this->session = cpr::Session();
        this->session.SetUserAgent("Mozilla/5.0");
    }

    void Rss::subscribe(const std::vector<std::string>& rss_links) {
        for (const std::string& link : rss_links) {
            this->rss_links.emplace_back(cpr::Url{link});
        }
    }

    // get RSS refreshs at {refresh_rate or ttl tag in xml per response} intervals

    // iterate over rss subscriptions and sends get requests respectively
    std::vector<cpr::Response> Rss::get_all_rss() {
        std::vector<cpr::Response> result;
        for (const cpr::Url& link : this->rss_links) {
            session.SetUrl(link);
            cpr::Response res = session.Get();
            if (res.status_code == 200) result.emplace_back(res);
            else
                std::cerr << "failed to reach RSS: " << link.str();
        }
        return result;
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
            article.content = crawl_article(item.child_value("link"));
            article.publisher = item.child("source") ? item.child_value("source") : domain;
            article.published_at = dt_utils::parse_rfc(item.child_value("pubDate"));
            articles.push_back(article);
        }

        return articles;
    }

    // crawl rss sublink
    std::string Rss::crawl_article(std::string link) {

    }

}; // namespace news
