#include "temporal.h"
#include "error.h"



//~ Unix
#if defined(TOOL_UNIX)

#include <time.h>

//~ Windows
#elif defined(TOOL_WINDOWS)

#include <Windows.h>

static i64 GetPerformanceFrequency()
{
    i64 frequency = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    return frequency;
}

#endif



namespace Tool
{
    //- Platform-agnostic

    i64 NanosecondsSince(Timepoint then)
    {
        return NanosecondsFromTo(then, TimepointNow());
    }

    i64 ClocksFromTo(Timepoint from, Timepoint to)
    {
        return to - from;
    }

    i64 ClocksSince(Timepoint then)
    {
        return TimepointNow() - then;
    }

    // Calendar from day count, after Howard Hinnant's civil_from_days, shifted to count from 1601-01-01, which was a
    // Monday. Years run March to February internally, so the leap day falls at the end of one.
    ClockTime ClockTimeFromSystemTimepoint(SystemTimepoint timepoint)
    {
        constexpr u64 ticksPerMillisecond = 10000;
        constexpr u64 millisecondsPerDay = 86400000;

        u64 milliseconds = timepoint / ticksPerMillisecond;
        u64 days = milliseconds / millisecondsPerDay;
        u64 dayMilliseconds = milliseconds % millisecondsPerDay;

        u64 shifted = days + 584694; // Days since 0000-03-01
        u64 era = shifted / 146097; // 400-year cycles
        u64 dayOfEra = shifted % 146097;
        u64 yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
        u64 dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
        u64 monthFromMarch = (5 * dayOfYear + 2) / 153;
        u64 month = monthFromMarch < 10 ? monthFromMarch + 3 : monthFromMarch - 9;

        ClockTime time = {};
        time.year = (u16)(era * 400 + yearOfEra + (month <= 2));
        time.month = (u16)month;
        time.weekday = (Weekday)(days % 7);
        time.day = (u16)(dayOfYear - (153 * monthFromMarch + 2) / 5 + 1);

        time.hour = (u16)(dayMilliseconds / 3600000);
        time.minute = (u16)(dayMilliseconds / 60000 % 60);
        time.second = (u16)(dayMilliseconds / 1000 % 60);
        time.millisecond = (u16)(dayMilliseconds % 1000);
        return time;
    }

    ClockDuration ClockDurationFromTo(SystemTimepoint from, SystemTimepoint to)
    {
        TOOL_DEBUG_ASSERT(from <= to, "The span ends before it starts.");

        u64 milliseconds = (to - from) / 10000;

        ClockDuration duration = {};
        duration.milliseconds = (u16)(milliseconds % 1000);
        duration.seconds = (u16)(milliseconds / 1000 % 60);
        duration.minutes = (u16)(milliseconds / 60000 % 60);
        duration.hours = (u16)(milliseconds / 3600000 % 24);
        duration.days = (u32)(milliseconds / 86400000);
        return duration;
    }

    //- Module

    //~ Module Windows implementation

#if TOOL_WINDOWS

    Timepoint TimepointNow()
    {
        Timepoint clocks = 0;
        QueryPerformanceCounter((LARGE_INTEGER*)&clocks);
        return clocks;
    }

    i64 NanosecondsFromTo(Timepoint from, Timepoint to)
    {
        constexpr i64 tenMHz = 10000000;
        constexpr i64 nsPerS = 1000000000;
        static const i64 frequency = GetPerformanceFrequency();

        i64 cycles = to - from;

        // Optimizing for the very common 10 MHz frequency
        if (frequency == tenMHz)
        {
            constexpr i64 multiplier = nsPerS / tenMHz;
            return cycles * multiplier;
        }
        else
        {
            i64 whole = (cycles / frequency) * nsPerS;
            i64 part = (cycles % frequency) * nsPerS / frequency;
            return whole + part;
        }
    }

    SystemTimepoint SystemTimepointNow(b8 local)
    {
        SystemTimepoint timepoint = {};
        FILETIME* fileTime = (FILETIME*)&timepoint;

        if (local)
        {
            SYSTEMTIME systemTime = {};
            GetLocalTime(&systemTime);
            SystemTimeToFileTime(&systemTime, fileTime);
        }
        else
        {
            GetSystemTimeAsFileTime(fileTime);
        }

        return timepoint;
    }

    ClockTime ClockTimeNow(b8 local)
    {
        SYSTEMTIME systemTime = {};

        if (local)
        {
            GetLocalTime(&systemTime);
        }
        else
        {
            GetSystemTime(&systemTime);
        }

        return
        {
            .year = systemTime.wYear,
            .month = systemTime.wMonth,
            .weekday = (Weekday)((systemTime.wDayOfWeek + 6) % 7), // SYSTEMTIME counts from Sunday
            .day = systemTime.wDay,

            .hour = systemTime.wHour,
            .minute = systemTime.wMinute,
            .second = systemTime.wSecond,
            .millisecond = systemTime.wMilliseconds
        };
    }

#endif // TOOL_WINDOWS

    //~ Module Unix implementation

#if TOOL_UNIX

    // Timepoints are nanoseconds, so clocks and nanoseconds are the same.
    Timepoint TimepointNow()
    {
        timespec now = {};
        clock_gettime(CLOCK_MONOTONIC, &now);
        return (i64)now.tv_sec * 1000000000 + now.tv_nsec;
    }

    i64 NanosecondsFromTo(Timepoint from, Timepoint to)
    {
        return to - from;
    }

    SystemTimepoint SystemTimepointNow(b8 local)
    {
        constexpr i64 secondsFrom1601To1970 = 11644473600;

        timespec now = {};
        clock_gettime(CLOCK_REALTIME, &now);

        i64 seconds = (i64)now.tv_sec;
        if (local)
        {
            time_t utc = now.tv_sec;
            tm calendar = {};
            localtime_r(&utc, &calendar);
            seconds += calendar.tm_gmtoff;
        }

        return (SystemTimepoint)(seconds + secondsFrom1601To1970) * 10000000 + (SystemTimepoint)now.tv_nsec / 100;
    }

    ClockTime ClockTimeNow(b8 local)
    {
        return ClockTimeFromSystemTimepoint(SystemTimepointNow(local));
    }

#endif // TOOL_UNIX

}
