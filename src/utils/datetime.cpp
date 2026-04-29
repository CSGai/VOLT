#include <ctime>
#include <iomanip>
#include <sstream>

// gets windows/unix version of gmt time
time_t toUtc(std::tm& t) {
#if defined(_WIN32)
    return _mkgmtime(&t);
#else
    return timegm(&t);
#endif
}

namespace dt_utils {
    // parses RFC822/RFC3339
    time_t parse_rfc(const std::string& dateStr) {
        std::tm t = {};
        std::istringstream ss(dateStr);

        // RFC 3339 starts with a digit e.g. "2026-04-29T10:00:00Z"
        // RFC 822 starts with a day name e.g. "Wed, 29 Apr 2026..."
        if (std::isdigit(dateStr[0])) {
            // RFC 3339
            ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");

            std::string tz = dateStr.substr(dateStr.rfind('T') + 9); // after seconds
            if (tz == "Z" || tz == "z") {
                return toUtc(t);
            }
            // numeric offset ("+05:30" 3339 uses colon)
            int sign = (tz[0] == '+') ? 1 : -1;
            int hours = std::stoi(tz.substr(1, 2));
            int mins = std::stoi(tz.substr(4, 2)); // skip colon
            time_t unix = toUtc(t);
            unix -= sign * (hours * 3600 + mins * 60);
            return unix;
        } else {
            // RFC 822
            ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S");

            std::string tz = dateStr.substr(dateStr.rfind(' ') + 1);
            if (tz == "GMT" || tz == "UTC") {
                return toUtc(t);
            }
            // numeric offset ("-0400" 822 has no colon)
            int sign = (tz[0] == '+') ? 1 : -1;
            int hours = std::stoi(tz.substr(1, 2));
            int mins = std::stoi(tz.substr(3, 2));
            time_t unix = toUtc(t);
            unix -= sign * (hours * 3600 + mins * 60);
            return unix;
        }
    }
} // namespace dt_utils
