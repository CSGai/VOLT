#include "utils.h"
#include <ctime>

namespace gutils::dt {

    void unix2datetime_gmt(time_t* unix_tstamp, tm* dt) { gmtime_s(dt, unix_tstamp); }
    void unix2datetime_local(time_t* unix_tstamp, tm* dt) { localtime_s(dt, unix_tstamp); }

    void datetime2unix(tm* dt, time_t* unix_tstamp) { *unix_tstamp = mktime(dt); }
} // namespace gutils::dt
