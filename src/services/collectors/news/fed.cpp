#include "news.hpp"
#include "utils/utils.hpp"

#include <cpr/cpr.h>

#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

static const std::string FRED_RELEASE_BASE = "https://api.stlouisfed.org/fred/release/dates";
static const std::string BLS_BASE = "https://api.bls.gov/publicAPI/v2/timeseries/data/";
static const std::string BEA_BASE = "https://apps.bea.gov/api/data/";
static const std::string NYFED_RATES_BASE = "https://markets.newyorkfed.org/api/rates/all/latest.json";

static const cpr::Header FED_HEADERS = {
    {"User-Agent",
     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
     "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"},
};

namespace news::fed {

static const std::unordered_map<Indicator, std::string> BLS_SERIES = {
    {Indicator::CPI, "CUUR0000SA0"},
    {Indicator::CoreCPI, "CUUR0000SA0L1E"},
    {Indicator::PPI, "WPUFD49104"},
    {Indicator::Unemployment, "LNS14000000"},
    {Indicator::Payrolls, "CES0000000001"},
};

struct BeaMeta {
    std::string table;
    std::string freq;
    std::string desc_filter;
};

static const std::unordered_map<Indicator, BeaMeta> BEA_META = {
    {Indicator::GDP, {"T10101", "Q", "Gross domestic product"}},
    {Indicator::PCE, {"T20804", "M", "Personal consumption expenditures"}},
    {Indicator::CorePCE, {"T20804", "M", "Excluding food and energy"}},
};

static const std::vector<std::pair<Indicator, std::string>> INDICATOR_MAP = {
    {Indicator::CPI, "CPI"},
    {Indicator::CoreCPI, "CoreCPI"},
    {Indicator::PPI, "PPI"},
    {Indicator::Unemployment, "Unemployment"},
    {Indicator::Payrolls, "Payrolls"},
    {Indicator::GDP, "GDP"},
    {Indicator::PCE, "PCE"},
    {Indicator::CorePCE, "CorePCE"},
    {Indicator::FedFundsRate, "FedFundsRate"},
};

static int current_year();
static cpr::Response get_bls(Indicator ind, const std::string& key, int count);
static cpr::Response get_bea(Indicator ind, const std::string& key, int count);
static cpr::Response get_effr();

// public interface
cpr::Response get_fomc_calendar(const std::string& fred_key) {
    cpr::Response res = cpr::Get(cpr::Url{FRED_RELEASE_BASE},
                                 FED_HEADERS,
                                 cpr::Parameters{
                                     {"release_id", "101"},
                                     {"api_key", fred_key},
                                     {"sort_order", "asc"},
                                     {"include_release_dates_with_no_data", "true"},
                                     {"file_type", "json"},
                                 });
    if (res.status_code != 200)
        return res;

    auto body = json::parse(res.text, nullptr, false);
    if (body.is_discarded() || !body.contains("release_dates"))
        return res;

    // UTC today same as std::chrono::system_clock::now();
    std::time_t today = (std::time(nullptr) / 86400) * 86400;

    json upcoming = json::array();
    for (const auto& entry : body["release_dates"]) {
        if (!entry.contains("date"))
            continue;

        const std::string& date = entry["date"].get<std::string>();
        std::tm mt{};

        std::cout << "entry: " << entry;

        sscanf_s(date.c_str(), "%d-%d-%d", &mt.tm_year, &mt.tm_mon, &mt.tm_mday);
        mt.tm_year -= 1900;
        mt.tm_mon -= 1;

        std::time_t meeting_t = utils::datetime::to_utc(mt);
        int days_until = (int)((meeting_t - today) / 86400);
        if (days_until < 0)
            continue;

        upcoming.push_back({{"date", date}, {"days_until", days_until}});
    }
    res.text = upcoming.dump();
    return res;
}

cpr::Response get_indicator(Indicator indicator, const ApiKeys& keys, int count) {
    switch (indicator) {
    case Indicator::CPI:
    case Indicator::CoreCPI:
    case Indicator::PPI:
    case Indicator::Unemployment:
    case Indicator::Payrolls:
        return get_bls(indicator, keys.bls, count);
    case Indicator::GDP:
    case Indicator::PCE:
    case Indicator::CorePCE:
        return get_bea(indicator, keys.bea, count);
    case Indicator::FedFundsRate:
        return get_effr();
    }
    cpr::Response err;
    err.status_code = 400;
    err.text = "unknown indicator";
    return err;
}

cpr::Response get_indicators(const ApiKeys& keys, int count) {

    json result = json::object();
    for (const auto& [indicator, name] : INDICATOR_MAP) {
        cpr::Response r = get_indicator(indicator, keys, count);
        if (r.status_code != 200)
            continue;
        auto data = json::parse(r.text, nullptr, false);
        if (!data.is_discarded())
            result[name] = data;
    }
    cpr::Response out;
    out.status_code = 200;
    out.text = result.dump();
    return out;
}

// utilities
static cpr::Response get_bls(Indicator indicator, const std::string& key, int count) {
    auto it = BLS_SERIES.find(indicator);
    if (it == BLS_SERIES.end()) {
        cpr::Response err;
        err.status_code = 400;
        err.text = "not a BLS indicator";
        return err;
    }
    int year = current_year();
    json body = {{"seriesid", json::array({it->second})},
                 {"startyear", std::to_string(year - 1)},
                 {"endyear", std::to_string(year)}};
    if (!key.empty()) body["registrationkey"] = key;
    cpr::Response res = cpr::Post(cpr::Url{BLS_BASE},
                                  cpr::Header{{"Content-Type", "application/json"}},
                                  cpr::Body{body.dump()});
    if (res.status_code != 200)
        return res;

    auto res_body = json::parse(res.text, nullptr, false);
    if (res_body.is_discarded())
        return res;

    // BLS v2 returns data newest-first
    const auto& data = res_body["Results"]["series"][0]["data"];
    json out = json::array();
    for (int i = 0; i < count && i < (int)data.size(); ++i)
        out.push_back({{"period", data[i]["year"].get<std::string>() + "-" + data[i]["period"].get<std::string>()},
                       {"value", data[i]["value"]}});
    res.text = out.dump();
    return res;
}

static cpr::Response get_bea(Indicator ind, const std::string& key, int count) {
    auto it = BEA_META.find(ind);
    if (it == BEA_META.end()) {
        cpr::Response err;
        err.status_code = 400;
        err.text = "not a BEA indicator";
        return err;
    }
    int year = current_year();
    const BeaMeta& m = it->second;
    cpr::Response res = cpr::Get(cpr::Url{BEA_BASE},
                                 FED_HEADERS,
                                 cpr::Parameters{
                                     {"UserID", key},
                                     {"method", "GetData"},
                                     {"DataSetName", "NIPA"},
                                     {"TableName", m.table},
                                     {"Frequency", m.freq},
                                     {"Year", std::to_string(year - 1) + "," + std::to_string(year)},
                                     {"ResultFormat", "JSON"},
                                 });
    if (res.status_code != 200)
        return res;

    auto body = json::parse(res.text, nullptr, false);
    if (body.is_discarded())
        return res;

    // BEA returns chronological order; collect matching lines then take last `count`
    const auto& data = body["BEAAPI"]["Results"]["Data"];
    std::vector<json> matches;
    for (const auto& row : data) {
        if (row.value("LineDescription", "").find(m.desc_filter) != std::string::npos)
            matches.push_back({{"period", row["TimePeriod"]}, {"value", row["DataValue"]}});
    }

    int start = (int)matches.size() > count ? (int)matches.size() - count : 0;
    json out = json::array();
    for (int i = start; i < (int)matches.size(); ++i)
        out.push_back(matches[i]);

    res.text = out.dump();
    return res;
}

static cpr::Response get_effr() {
    cpr::Response res = cpr::Get(cpr::Url{NYFED_RATES_BASE}, FED_HEADERS);
    if (res.status_code != 200)
        return res;

    auto body = json::parse(res.text, nullptr, false);
    if (body.is_discarded())
        return res;

    for (const auto& rate : body["refRates"]) {
        if (rate.value("type", "") == "EFFR") {
            res.text =
                json{
                    {"date", rate["effectiveDate"]},
                    {"rate", rate["percentRate"]},
                    {"target_from", rate["targetRateFrom"]},
                    {"target_to", rate["targetRateTo"]},
                }
                    .dump();
            return res;
        }
    }
    res.status_code = 404;
    res.text = "EFFR not in response";
    return res;
}

static int current_year() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    return tm.tm_year + 1900;
}

} // namespace news::fed
