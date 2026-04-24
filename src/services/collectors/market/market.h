#pragma once

#include "cpr/parameters.h"
#include "cpr/response.h"
#include <cpr/cpr.h>
#include <string>
#include <unordered_map>

namespace api {

    // d -> days
    // m -> minutes
    enum intervals { _1m, _2m, _5m, _15m, _30m, _60m, _90m, _1d, _5d, _1wk, _1mo, _3mo };

    static const std::unordered_map<api::intervals, std::string>& INTERVAL_MAP = {
        {api::_1m, "1m"},   {api::_2m, "2m"}, {api::_5m, "5m"}, {api::_15m, "15m"}, {api::_30m, "30m"}, {api::_60m, "60m"},
        {api::_90m, "90m"}, {api::_1d, "1d"}, {api::_5d, "5d"}, {api::_1wk, "1wk"}, {api::_1mo, "1mo"}, {api::_3mo, "3mo"}};

    // query yahoo api
    class Yahoo {
        public:
            Yahoo();
            cpr::Response get_hist(const std::string& ticker, const time_t& period1, const time_t& period2,
                                   const api::intervals& interval);
            cpr::Response get_quote(const std::vector<std::string>& symbols);
            cpr::Response get_ticker_news(const std::vector<std::string>& symbols, const int& news_count);
            std::string get_crumb();

        private:
            cpr::Session session;
            std::string host;
            std::string crumb;

            cpr::Response Get(const std::string& url, cpr::Parameters params = cpr::Parameters{}, cpr::Header headers = cpr::Header{});

            // util
            static std::string test_urls();
    };

}; // namespace api
