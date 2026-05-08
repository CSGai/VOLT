#include "market.hpp"
#include "utils/utils.hpp"
#include <cpr/cpr.h>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// const links and endpoints
static const std::string Q1_YAHOO = "https://query1.finance.yahoo.com";
// Q2 is less stable than Q1
static const std::string Q2_YAHOO = "https://query2.finance.yahoo.com";

static const std::string QYAHOO_HIST = "/v8/finance/chart/";
static const std::string QYAHOO_QUOTE = "/v7/finance/quote";
static const std::string QYAHOO_CRUMBS = "/v1/test/getcrumb";

// q/s -> ticker for relevent news
// newsCount -> amount of articles to get
static const std::string QYAHOO_SEARCH = "/v1/finance/search";

namespace api {

    Yahoo::Yahoo() {

        // session setup
        host = test_urls();
        session = cpr::Session();
        session.SetUserAgent("Mozilla/5.0");
        // grab cookies for step 1 authentication
        session.SetUrl("https://fc.yahoo.com");
        session.Get();
    }

    // get ticker's historical data
    cpr::Response Yahoo::get_hist(const std::string& ticker, const time_t& period1, const time_t& period2,
                                  const api::intervals& interval) {
        cpr::Parameters params = cpr::Parameters{
            {"period1", std::to_string(period1)}, {"period2", std::to_string(period2)}, {"interval", api::INTERVAL_MAP.at(interval)}};
        return Get(host + QYAHOO_HIST + ticker, params);
    }

    // get ticker's real time data
    cpr::Response Yahoo::get_quote(const std::vector<std::string>& symbols) {
        // get crum for step 2 authentication
        if (this->crumb.empty()) {
            get_crumb();
        }

        cpr::Parameters params = cpr::Parameters{{"symbols", utils::misc::str_join(symbols, ',')}, {"crumb", this->crumb}};
        cpr::Response res = Get(host + QYAHOO_QUOTE, params);
        if (res.status_code != 200) {
            this->crumb = get_crumb();
            res = Get(host + QYAHOO_QUOTE, params);
        }
        return res;
    }

    cpr::Response Yahoo::get_ticker_news(const std::vector<std::string>& symbols, int news_count) {
        cpr::Parameters params = cpr::Parameters{{"q", utils::misc::str_join(symbols, ',')}, {"newsCount", std::to_string(news_count)}};
        return Get(host + QYAHOO_SEARCH, params);
    }

    // session get
    cpr::Response Yahoo::Get(const std::string& url, cpr::Parameters params, cpr::Header headers) {
        // session set variables
        if (!headers.empty()) session.SetHeader(headers);

        session.SetUrl(cpr::Url{url});
        session.SetParameters(params);

        return session.Get();
    }

    // get crumbs for authorization
    std::string Yahoo::get_crumb() {

        session.SetUrl(cpr::Url{host + QYAHOO_CRUMBS});
        session.SetParameters({});

        const cpr::Response crumb_response = session.Get();

        crumb = crumb_response.text;
        return crumb;
    }

    // test API urls Query1/Query2
    std::string Yahoo::test_urls() {
        // refuses to connect without user-agent
        static const cpr::Header user_agent{{"User-Agent", "Mozilla/5.0"}};
        static const std::string params = "PLTR?period1=1699549200&period2=1731319200&interval=1d";

        // test endpoints
        for (const std::string& base_url : {Q1_YAHOO, Q2_YAHOO}) {
            cpr::Response r = cpr::Get(cpr::Url{base_url + QYAHOO_HIST + params}, user_agent);

            if (r.status_code == 200) {
                std::cout << "Success: " << base_url << r.status_line << std::endl;
                return base_url;
            }
            std::cout << "FAILED " << base_url << " HIST: " << r.status_line << std::endl;
        }

        std::cerr << "All Yahoo Finance endpoints failed";
        return NULL;
    }



} // namespace api