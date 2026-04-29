#include "news.h"
#include "utils/utils.h"
#include <cpr/cpr.h>
#include <string>

namespace news {
    Rss::Rss(const std::vector<std::string>& rss_links) {
        for (const std::string& link : rss_links) {
            this->rss_links.emplace_back(cpr::Url{link});
        }
    }
    // iterate over rss links to get rss stuff

    // parse xml and extract wanted information

    // crawl rss sublinks if possible

}; // namespace news
