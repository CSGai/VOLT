#include "news.hpp"
#include "utils/utils.hpp"

#include <cpr/cpr.h>

#include <iostream>
#include <pugixml.hpp>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int default_refresh_rate = 15;

namespace news {
static const cpr::Header default_headers = {{"User-Agent", "Mozilla/5.0"}};

void Rss::subscribe(const std::vector<std::string>& rss_links) {
    for (const std::string& link : rss_links) {
        this->rss_links.emplace_back(cpr::Url{link});
    }
    set_refresh_rates();
}

void Rss::run(int interval, Destination) {
    // call and process data from get_rss based on intervals
    const auto& urls = this->refresh_rates[interval];

    std::vector<cpr::AsyncWrapper<cpr::Response>> futures;
    futures.reserve(urls.size());

    for (const auto& url : urls)
        futures.emplace_back(get_rss(url));

    // write data to cache
    for (auto& f : futures) {
        auto result = f.get();
        // result.text;
        // ADD socket based IPC
    }
}

void Rss::print_refresh_rates() {
    for (const auto& [ttl, urls] : this->refresh_rates) {
        for (const auto& url : urls) {
            std::cout << ttl << ": " << url << "\n";
        }
    }
}

// fire all TTL probes in parallel, then collect
void Rss::set_refresh_rates() {
    std::vector<cpr::AsyncWrapper<std::pair<std::string, int>>> futures;
    futures.reserve(this->rss_links.size());

    for (const cpr::Url& link : this->rss_links) {
        futures.push_back(cpr::async([this, link]() -> std::pair<std::string, int> {
            cpr::Response res = cpr::Get(link, default_headers);
            if (res.status_code != 200) {
                std::cerr << "failed to reach RSS: " << link.str();
                return {link.str(), default_refresh_rate};
            }
            pugi::xml_document doc;
            doc.load_string(res.text.c_str());
            auto channel = doc.child("rss").child("channel");
            return {res.url.str(), parse_ttl(channel.child_value("ttl"))};
        }));
    }

    std::unordered_map<int, std::vector<std::string>> result;
    for (auto& f : futures) {
        auto [url, ttl] = f.get();
        result[ttl].emplace_back(url);
    }
    this->refresh_rates = std::move(result);
}

// sync wrapper around get_rss; exists only as a test seam for the 304 path
cpr::Response Rss::fetch_sync(cpr::Url link) {
    return get_rss(link).get();
}

// async fetch with conditional headers (ETag / If-Modified-Since)
cpr::AsyncWrapper<cpr::Response> Rss::get_rss(cpr::Url link) {
    return cpr::async([this, link]() {
        cpr::Header headers = default_headers;
        {
            std::lock_guard<std::mutex> lk(cache_mutex);
            auto cached = cache_validators.find(link.str());
            if (cached != cache_validators.end()) {
                if (!cached->second.etag.empty())
                    headers["If-None-Match"] = cached->second.etag;
                if (!cached->second.last_modified.empty())
                    headers["If-Modified-Since"] = cached->second.last_modified;
            }
        }

        cpr::Response res = cpr::Get(link, headers);

        if (res.status_code == 200) {
            std::lock_guard<std::mutex> lk(cache_mutex);
            CacheValidators& v = cache_validators[link.str()];
            auto et = res.header.find("ETag");
            if (et != res.header.end())
                v.etag = et->second;
            auto lm = res.header.find("Last-Modified");
            if (lm != res.header.end())
                v.last_modified = lm->second;
        } else if (res.status_code != 304) {
            std::cerr << "failed to reach RSS: " << link.str();
        }

        return res;
    });
}

// parse ttl value
int Rss::parse_ttl(const std::string& ttl_str) {
    if (ttl_str.empty() || ttl_str[0] == '\0')
        return default_refresh_rate;
    int ttl = std::stoi(ttl_str);
    if (ttl <= 0)
        return default_refresh_rate;
    // cap at 1 hour
    if (ttl > 60)
        return 60;
    return ttl;
}

// parse xml and extract wanted information into articles
std::vector<news::Article> Rss::parse_rss(const cpr::Response& rss_res) {

    std::string url = rss_res.url.str();
    // get '/' after initial :// index
    size_t path = url.find('/', url.find("://") + 3);
    std::string domain = (path == std::string::npos) ? url + "/" : url.substr(0, path + 1);

    pugi::xml_document xml;
    xml.load_string(rss_res.text.c_str());

    // extract articles
    std::vector<news::Article> articles;
    auto channel = xml.child("rss").child("channel");
    for (auto item : channel.children("item")) {
        news::Article article;
        article.title = item.child_value("title");
        article.url = item.child_value("link");
        article.content = item.child_value("description");
        // article.content = crawl_article(item.child_value("link"));
        auto pub = item.child("source");
        article.publisher = pub ? pub.child_value() : domain;
        article.published_at = utils::datetime::parse_rfc(item.child_value("pubDate"));
        articles.push_back(article);
    }

    return articles;
}

// crawl rss sublink
// std::string Rss::crawl_article(const std::string& link) {
//     session.SetUrl(link);
//     std::string html = session.Get().text;
//     return "";
// }

}; // namespace news
