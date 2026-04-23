#pragma once

#include <cpr/cpr.h>
#include <string>
#include <unordered_map>

namespace market {
    /*
    1m data is usually only available for the last ~7 days
    2m/5m/15m also have limited history windows
    */
    enum intervals {
        // m -> minutes
        _1m,
        _2m,
        _5m,
        _15m,
        _30m,
        _60m,
        _90m,
        // d -> days
        _1d,
        _5d,
        _1wk,
        _1mo,
        _3mo
    };

    class QueryYahoo {
        public:
            QueryYahoo();

            cpr::Response get_hist(const std::string& ticker, const time_t& period1, const time_t& period2,
                                   const market::intervals& interval);
            cpr::Response get_rt(const std::vector<std::string>& symbols);

        private:
            cpr::Session session;
            cpr::Url url;
            std::string crumb;

            cpr::Response Get(const std::string& endpoint, cpr::Parameters params, cpr::Header headers = cpr::Header{});

            std::string get_crumb();
            static cpr::Url test_urls();
    };

}; // namespace market

static std::unordered_map<market::intervals, std::string> INTERVAL_MAP = {
    {market::_1m, "1m"},   {market::_2m, "2m"},   {market::_5m, "5m"},   {market::_15m, "15m"},
    {market::_30m, "30m"}, {market::_60m, "60m"}, {market::_90m, "90m"}, {market::_1d, "1d"},
    {market::_5d, "5d"},   {market::_1wk, "1wk"}, {market::_1mo, "1mo"}, {market::_3mo, "3mo"}};

static const std::string Q1_YAHOO = "https://query1.finance.yahoo.com";
static const std::string Q2_YAHOO = "https://query2.finance.yahoo.com";

static const std::string QYAHOO_HIST = "/v8/finance/chart/";
static const std::string QYAHOO_QUOTE = "/v7/finance/quote";
static const std::string QYAHOO_CRUMBS = "/v1/test/getcrumb";