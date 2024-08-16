#ifndef TOOL_TEMPORAL_H
#define TOOL_TEMPORAL_H

#include "basics.h"



namespace Tool
{
    //- Type definitions
    
    //~ Monotonic performance clock
    
    // Represents performance counter clocks on Windows, nanoseconds on Linux. Access through utility functions.
    typedef i64 Timepoint;
    
    struct Duration
    {
        i32 seconds;
        i32 nanoseconds;
    };
    
    //~ Wall/system clock
    // Only precise to seconds on Linux
    
    enum Weekday
    {
        Monday = 0,
        Tuesday = 1,
        Wednesday = 2,
        Thursday = 3,
        Friday = 4,
        Saturday = 5,
        Sunday = 6
    };
    
    // TODO(crazy): Implement
    struct SystemTimepoint
    {
        i16 year;
        i16 month;
        Weekday weekday;
        i16 day;
        
        i16 hour;
        i16 minute;
        i16 second;
        i16 millisecond;
        
        u64 raw;
    };
    
    // TODO(crazy): Implement
    struct SystemDuration
    {
        i16 years;
        i16 months;
        i16 days;
        i16 hours;
        i16 minutes;
        i16 seconds;
        i16 milliseconds;
    };
    
    
    
    //- Function definitions
    
    //~ Monotonic measurement
    
    Timepoint TimepointNow();
    
    i64 NanosecondsFromTo(Timepoint from, Timepoint to);
    i64 NanosecondsSince(Timepoint then);
    
    i64 ClocksFromTo(Timepoint from, Timepoint to);
    i64 ClocksSince(Timepoint then);
    
    template<typename T>
        T SecondsFromTo(Timepoint from, Timepoint to) { return NanosecondsFromTo(from, to) / ((T)1000000000); }
    
    template<typename T>
        T SecondsSince(Timepoint then) { return NanosecondsSince(then) / ((T)1000000000); }
};


#endif //TOOL_TEMPORAL_H
