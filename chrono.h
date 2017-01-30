#pragma once

#include <chrono>


#if defined _MSC_VER && _MSC_VER < 1900
#define AW_IO_BAD_WINDOWS_CHRONO_HIGH_RESOLUTION_CLOCK
#endif

#ifdef AW_IO_BAD_WINDOWS_CHRONO_HIGH_RESOLUTION_CLOCK

// VS2013 has no high resolution clock in std::chrono, this is a fix

class windows_qpc_clock
{
public:
    using duration = std::chrono::nanoseconds;
    using period = duration::period;
    using time_point = std::chrono::time_point<windows_qpc_clock>;
    using rep = duration::rep;
    static const bool is_steady = true;
    static time_point now();
};

using high_resolution_clock = windows_qpc_clock;
using steady_clock = windows_qpc_clock;

#else

using steady_clock = std::chrono::steady_clock;
using high_resolution_clock = std::chrono::high_resolution_clock;

#endif
