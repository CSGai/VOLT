#pragma once
#include <ctime>
#include <string>
#include <vector>

namespace utils::datetime {

/// parse rfc822 and rfc3339 to a unix timestamp
time_t parse_rfc(const std::string& date_str);

/// convert a std::tm (interpreted as UTC) to time_t
time_t to_utc(std::tm& t);

} // namespace utils::datetime

namespace utils::misc {

/// join string on delimiter
std::string str_join(const std::vector<std::string>& symbols, const char& delimiter);

/// remove substring from string
std::string str_remove(std::string str, const std::string& sub);

} // namespace utils::misc