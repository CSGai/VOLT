#pragma once

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

void test_endpoints(std::string ticker, time_t period1, time_t period2,
                    market::intervals interval);
std::string test_analysis();
}; // namespace market

static std::unordered_map<market::intervals, std::string> INTERVAL_MAP = {
    {market::_1m, "1m"},   {market::_2m, "2m"},   {market::_5m, "5m"},
    {market::_15m, "15m"}, {market::_30m, "30m"}, {market::_60m, "60m"},
    {market::_90m, "90m"}, {market::_1d, "1d"},   {market::_5d, "5d"},
    {market::_1wk, "1wk"}, {market::_1mo, "1mo"}, {market::_3mo, "3mo"}};

const std::string YAHOO_ENDPOINT1 =
    "https://query1.finance.yahoo.com/v8/finance/chart/";
const std::string YAHOO_ENDPOINT2 =
    "https://query2.finance.yahoo.com/v8/finance/chart/";
