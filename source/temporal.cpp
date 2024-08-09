#include "temporal.h"



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
    
#endif // TOOL_WINDOWS
    
    //~ Module Unix implementation
    
#if TOOL_UNIX
    
    
    
#endif // TOOL_UNIX
    
}
