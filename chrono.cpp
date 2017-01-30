#include "chrono.h"

#include <cassert>

#ifdef AW_IO_BAD_WINDOWS_CHRONO_HIGH_RESOLUTION_CLOCK
#include <Windows.h>
#endif


#ifdef AW_IO_BAD_WINDOWS_CHRONO_HIGH_RESOLUTION_CLOCK

namespace {

class qpc_clock_qpfreq_check_t
{
public:
    qpc_clock_qpfreq_check_t()
    {
        LARGE_INTEGER f;
		auto b = QueryPerformanceFrequency(&f);
        assert(b);
        qpc_to_nsec = (int64_t)round(1e9 / f.QuadPart);
    }
    int64_t qpc_to_nsec;
} qpc_clock_qpfreq_check;
}

windows_qpc_clock::time_point windows_qpc_clock::now()
{
    LARGE_INTEGER c;
	auto b = QueryPerformanceCounter(&c);
    assert(b);
    auto nsec = c.QuadPart * qpc_clock_qpfreq_check.qpc_to_nsec;
    return time_point(duration(nsec));
}

#endif
