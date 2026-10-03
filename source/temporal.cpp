#include "temporal.h"
#include "error.h"



//- Platform-agnostic

//~ Unix
#if defined(TOOL_UNIX)

#include <time.h>
#include <sys/types.h>

//~ Windows
#elif defined(TOOL_WINDOWS)

#include <Windows.h>

i64 GetPerformanceFrequency()
{
    i64 frequency = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    return frequency;
}

#endif



namespace Tool
{
    //- Platform-agnostic
    
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
    
    i64 NanosecondsSince(Timepoint then)
    {
        return NanosecondsFromTo(then, TimepointNow());
    }
    
    i64 ClocksFromTo(Timepoint from, Timepoint to)
    {
        return to - from;;
    }
    
    i64 ClocksSince(Timepoint then)
    {
        return TimepointNow() - then;
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
    
    static inline ClockTime ClockTimeConvert(SYSTEMTIME* systemTime)
    {
        return
        {
            .year = systemTime->wYear,
            .month = systemTime->wMonth,
            .weekday = (Weekday)((systemTime->wDayOfWeek + 6) % 7), // SYSTEMTIME counts from Sunday
            .day = systemTime->wDay,
            
            .hour = systemTime->wHour,
            .minute = systemTime->wMinute,
            .second = systemTime->wSecond,
            .millisecond = systemTime->wMilliseconds
        };
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
        
        return ClockTimeConvert(&systemTime);
    }
    
    
    ClockTime ClockTimeFromSystemTimepoint(SystemTimepoint timepoint)
    {
        SYSTEMTIME systemTime = {};
        FileTimeToSystemTime((FILETIME*)&timepoint, &systemTime);
        
        return ClockTimeConvert(&systemTime);
    }
    
#endif // TOOL_WINDOWS
    
    //~ Module Unix implementation
    
#if TOOL_UNIX
    
    
    
#endif // TOOL_UNIX
    
}
