#include "utils.h"
#include <ctime>

namespace gutils {

void unix2datetime_gmt(char *buffer, size_t buffer_size, time_t unix_tstamp) {
  tm dt{};

  gmtime_s(&dt, &unix_tstamp);
  std::strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &dt);
}
void unix2datetime_local(char *buffer, size_t buffer_size, time_t unix_tstamp) {
  tm dt{};

  localtime_s(&dt, &unix_tstamp);
  std::strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &dt);
}

void datetime2unix(char *datetime) {}

} // namespace gutils