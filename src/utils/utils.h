#pragma once
#include <ctime>

namespace gutils {

void unix2datetime(char *buffer, size_t buffer_size, time_t unix_tstamp);
void datetime2unix(char *datetime);

} // namespace gutils