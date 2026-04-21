#include "market.h"

// #include "cpr/verbose.h"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/parameters.h"
#include "cpr/response.h"
#include "cpr/session.h"

#include <combaseapi.h>
#include <cpr/cpr.h>
#include <iostream>
#include <rpcndr.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace market {

    YahooAPI::YahooAPI() {

        // session setup
        this->url = test_urls();
        this->session = cpr::Session();
        // this->session.SetOption(cpr::Verbose(true));
        this->session.SetUserAgent("Mozilla/5.0");
        // grab cookies for step 1 authentication
        this->session.SetUrl("https://fc.yahoo.com");
        this->session.Get();
    }
    // get ticker's historical data
    cpr::Response YahooAPI::get_hist(const std::string& ticker, const time_t& period1, const time_t& period2,
                                     const market::intervals& interval) {
        return Get(YAHOO_HIST_ENDPOINT + ticker, cpr::Parameters{{"period1", std::to_string(period1)},
                                                                 {"period2", std::to_string(period2)},
                                                                 {"interval", INTERVAL_MAP[interval]}});
    }
    // get ticker's real time data
    cpr::Response YahooAPI::get_rt(const std::vector<std::string>& symbols) {
        // get crum for step 2 authentication
        if (this->crumb.empty()) {
            get_crumb();
        }
        // concat symbols with delimiter ','
        std::string str_symbols =
            std::accumulate(std::next(symbols.begin()), symbols.end(), symbols.empty() ? std::string{} : symbols.front(),
                            [](std::string a, const std::string& b) { return std::move(a) + ',' + b; });
        
        cpr::Parameters params = cpr::Parameters{{"symbols", str_symbols}, {"crumb", this->crumb}};
        cpr::Response res = Get(YAHOO_RT_ENDPOINT, params);
        if (res.status_code != 200) {
            this->crumb = get_crumb();
            res = Get(YAHOO_RT_ENDPOINT, params);
        }
        return res;
    }
    // session get
    cpr::Response YahooAPI::Get(const std::string& endpoint, cpr::Parameters params, cpr::Header headers) {
        // session set variables
        if (!headers.empty()) this->session.SetHeader(headers);

        this->session.SetUrl(url + endpoint);
        this->session.SetParameters(params);

        return this->session.Get();
    }
    // get crumbs for authorization
    std::string YahooAPI::get_crumb() {

        this->session.SetUrl(YAHOO_Q1 + YAHOO_CRUMBS);
        this->session.SetParameters({});

        const cpr::Response crumb_response = this->session.Get();

        this->crumb = crumb_response.text;
        return this->crumb;
    }
    // test API urls Query1/QueryQ2
    cpr::Url YahooAPI::test_urls() {
        // refuses to connect without user-agent
        static const cpr::Header user_agent{{"User-Agent", "Mozilla/5.0"}};
        static const std::string params = "PLTR?period1=1699549200&period2=1731319200&interval=1d";

        // test endpoints
        for (std::string base_url : {YAHOO_Q1, YAHOO_Q2}) {
            const std::string full_url = std::string(base_url) + YAHOO_HIST_ENDPOINT + params;
            cpr::Response r = cpr::Get(cpr::Url{full_url}, user_agent);

            if (r.status_code == 200) {
                std::cout << "Success " << base_url << " HIST: " << r.status_code << std::endl;
                return cpr::Url{std::string(base_url)};
            }
            std::cout << "FAILED " << base_url << " HIST: " << r.status_code << std::endl;
        }

        // Better error handling instead of NULL
        throw std::runtime_error("All Yahoo Finance endpoints failed");
    }

} // namespace market
