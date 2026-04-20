#pragma once
#include <ctime>

namespace gutils::dt {
    void unix2datetime_gmt(time_t* unix_tstamp, tm* dt);
    void unix2datetime_local(time_t* unix_tstamp, tm* dt);

    void datetime2unix(tm* dt, time_t* unix_tstamp);
} // namespace gutils::dt