#include "news.hpp"

#include <cpr/cpr.h>

#include <ctime>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

static const std::string FED_FOMC_CALENDAR =
    "https://www.federalreserve.gov/monetarypolicy/fomccalendars.json";

static const std::string FRED_BASE = "https://api.stlouisfed.org/fred/series/observations";

static const cpr::Header FED_HEADERS = {
    {"User-Agent",
     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
     "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36"},
};

namespace news::fed {

static const std::unordered_map<Indicator, std::string> FRED_SERIES = {
    {Indicator::CPI, "CPIAUCSL"},
    {Indicator::CoreCPI, "CPILFESL"},
    {Indicator::PCE, "PCEPI"},
    {Indicator::CorePCE, "PCEPILFE"},
    {Indicator::GDP, "GDP"},
    {Indicator::Unemployment, "UNRATE"},
    {Indicator::FedFundsRate, "FEDFUNDS"},
    {Indicator::PPI, "PPIACO"},
    {Indicator::Payrolls, "PAYEMS"},
};

// upcoming FOMC meetings with days_until each
cpr::Response get_fomc_calendar() {
    cpr::Response res = cpr::Get(cpr::Url{FED_FOMC_CALENDAR}, FED_HEADERS);
    if (res.status_code != 200)
        return res;

    auto body = json::parse(res.text, nullptr, false);
    if (body.is_discarded() || !body.contains("meetings"))
        return res;

    std::time_t now = std::time(nullptr);
    std::tm now_tm{};
    gmtime_s(&now_tm, &now);
    now_tm.tm_hour = 0;
    now_tm.tm_min = 0;
    now_tm.tm_sec = 0;
    std::time_t today = std::mktime(&now_tm);

    json upcoming = json::array();
    for (const auto& m : body["meetings"]) {
        if (!m.contains("year") || !m.contains("month") || !m.contains("day"))
            continue;

        const auto& days = m["day"];
        // last day of the meeting is when decisions are announced
        std::string last_day =
            days.is_array() ? days.back().get<std::string>() : days.get<std::string>();

        std::tm mt{};
        mt.tm_year = std::stoi(m["year"].get<std::string>()) - 1900;
        mt.tm_mon = std::stoi(m["month"].get<std::string>()) - 1;
        mt.tm_mday = std::stoi(last_day);
        std::time_t meeting_t = std::mktime(&mt);

        int days_until = (int)((meeting_t - today) / 86400);
        if (days_until < 0)
            continue;

        std::string mon_s = m["month"].get<std::string>();
        if (mon_s.size() == 1) mon_s = "0" + mon_s;
        std::string day_s = last_day;
        if (day_s.size() == 1) day_s = "0" + day_s;

        upcoming.push_back({
            {"date", m["year"].get<std::string>() + "-" + mon_s + "-" + day_s},
            {"days_until", days_until},
            {"has_press_conf", m.value("isMeetingWithPress", false)},
        });
    }

    res.text = upcoming.dump();
    return res;
}

// latest `count` FRED observations for one indicator
cpr::Response get_indicator(Indicator ind, const std::string& api_key, int count) {
    auto it = FRED_SERIES.find(ind);
    if (it == FRED_SERIES.end()) {
        cpr::Response err;
        err.status_code = 400;
        err.text = "unknown indicator";
        return err;
    }
    return cpr::Get(cpr::Url{FRED_BASE},
                    FED_HEADERS,
                    cpr::Parameters{
                        {"series_id", it->second},
                        {"api_key", api_key},
                        {"sort_order", "desc"},
                        {"limit", std::to_string(count)},
                        {"file_type", "json"},
                    });
}

// latest `count` observations for every indicator; res.text = {"CPI":[...], ...}
cpr::Response get_indicators(const std::string& api_key, int count) {
    json result = json::object();
    for (const auto& [ind, series_id] : FRED_SERIES) {
        cpr::Response r = get_indicator(ind, api_key, count);
        if (r.status_code != 200)
            continue;
        auto body = json::parse(r.text, nullptr, false);
        if (body.is_discarded() || !body.contains("observations"))
            continue;
        result[series_id] = body["observations"];
    }
    cpr::Response out;
    out.status_code = 200;
    out.text = result.dump();
    return out;
}

} // namespace news::fed
