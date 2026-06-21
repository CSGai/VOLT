#include "news.hpp"
#include "utils/utils.hpp"

#include <cpr/cpr.h>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <string>
#include <vector>

using json = nlohmann::json;

static const std::string FOMC_CAL = "https://www.federalreserve.gov/monetarypolicy/fomccalendars.htm";
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

static const std::vector<std::pair<int, std::string>> FRED_EVENTS = {
    {46, "CPI"},
    {10, "NFP / Employment Situation"},
    {51, "PCE / Personal Income & Outlays"},
    {22, "GDP"},
    {31, "PPI"},
};

static int current_year();
static std::optional<Event> scrape_fomc();
static cpr::Response get_bls(Indicator ind, const std::string& key, int count);
static cpr::Response get_bea(Indicator ind, const std::string& key, int count);
static cpr::Response get_effr();

// public interface
std::vector<Event> get_event_calendar(const std::string& fred_key) {
    std::time_t today = (std::time(nullptr) / 86400) * 86400;

    std::vector<Event> events;

    for (const auto& [release_id, label] : FRED_EVENTS) {
        cpr::Response res = cpr::Get(cpr::Url{FRED_RELEASE_BASE},
                                     FED_HEADERS,
                                     cpr::Parameters{
                                         {"release_id", std::to_string(release_id)},
                                         {"api_key", fred_key},
                                         {"sort_order", "asc"},
                                         {"include_release_dates_with_no_data", "true"},
                                         {"file_type", "json"},
                                     });
        if (res.status_code != 200)
            continue;

        auto body = json::parse(res.text, nullptr, false);
        if (body.is_discarded() || !body.contains("release_dates"))
            continue;

        for (const auto& entry : body["release_dates"]) {
            if (!entry.contains("date"))
                continue;

            const std::string& date = entry["date"].get<std::string>();
            std::tm mt{};

            sscanf_s(date.c_str(), "%d-%d-%d", &mt.tm_year, &mt.tm_mon, &mt.tm_mday);
            mt.tm_year -= 1900;
            mt.tm_mon -= 1;
            int days_until = (int)((utils::datetime::to_utc(mt) - today) / 86400);

            // only next upcoming per event type; skip today since FRED publication dates
            // can lag the actual event by days, making past events appear as today
            if (days_until > 0) {
                events.push_back({label, date, days_until});
                break;
            }
        }
    }

    if (auto fomc = scrape_fomc())
        events.push_back(*fomc);

    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.days_until < b.days_until; });
    
    return events;
}

std::optional<cpr::Response> get_indicator(Indicator indicator, const ApiKeys& keys, int count) {
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
    default:
        return std::nullopt;
    }
}

// utilities
static std::optional<Event> scrape_fomc() {
    static const std::string FOMC_CAL =
        "https://www.federalreserve.gov/monetarypolicy/fomccalendars.htm";

    cpr::Response res = cpr::Get(cpr::Url{FOMC_CAL}, FED_HEADERS);
    if (res.status_code != 200) return std::nullopt;

    const std::string& html = res.text;
    std::time_t today = (std::time(nullptr) / 86400) * 86400;
    int year = current_year();

    static const std::unordered_map<std::string, int> MONTHS = {
        {"January",1},{"February",2},{"March",3},{"April",4},
        {"May",5},{"June",6},{"July",7},{"August",8},
        {"September",9},{"October",10},{"November",11},{"December",12},
    };

    // Current year's meetings sit at the top of the page in document order.
    // Scan all rows: fomc-meeting__month div (<strong>MonthName</strong>)
    // followed by sibling fomc-meeting__date div (DD-DD).
    // Assign current year to every match; past meetings will have days_until <= 0
    // and are skipped, so the first positive match is the next upcoming meeting.
    static const std::regex ENTRY_RE(
        R"(fomc-meeting__month[^>]*><strong>(January|February|March|April|May|June|July|August|September|October|November|December)</strong></div>\s*<div[^>]*fomc-meeting__date[^>]*>(\d{1,2})-(\d{1,2}))");

    for (auto it = std::sregex_iterator(html.begin(), html.end(), ENTRY_RE);
         it != std::sregex_iterator{}; ++it) {
        auto mit = MONTHS.find((*it)[1].str());
        if (mit == MONTHS.end()) continue;
        int month     = mit->second;
        int start_day = std::stoi((*it)[2]);
        int end_day   = std::stoi((*it)[3]);

        std::tm mt{};
        mt.tm_year = year - 1900;
        mt.tm_mon  = month - 1;
        mt.tm_mday = start_day;
        int days_until = (int)((utils::datetime::to_utc(mt) - today) / 86400);
        if (days_until <= 0) continue;

        char buf[14];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d/%02d", year, month, start_day, end_day);
        return Event{"FOMC Rate Decision", buf, days_until};
    }

    return std::nullopt;
}

static cpr::Response get_bls(Indicator indicator, const std::string& key, int count) {
    auto it = BLS_SERIES.find(indicator);
    if (it == BLS_SERIES.end()) {
        cpr::Response err;
        err.status_code = 400;
        err.text = "not a BLS indicator";
        return err;
    }
    int year = current_year();
    json body = {{"seriesid", json::array({it->second})}, {"startyear", std::to_string(year - 1)}, {"endyear", std::to_string(year)}};
    if (!key.empty())
        body["registrationkey"] = key;
    cpr::Response res = cpr::Post(cpr::Url{BLS_BASE}, cpr::Header{{"Content-Type", "application/json"}}, cpr::Body{body.dump()});
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
