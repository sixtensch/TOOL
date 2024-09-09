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
    
    enum Weekday : u16
    {
        Monday = 0,
        Tuesday = 1,
        Wednesday = 2,
        Thursday = 3,
        Friday = 4,
        Saturday = 5,
        Sunday = 6
    };
    
    // This represents a large continuous number
    // Can be operated on arithmetically
    typedef u64 SystemTimepoint;
    
    // TODO(crazy): Implement
    struct ClockTime
    {
        u16 year;
        u16 month;
        Weekday weekday;
        u16 day;
        
        u16 hour;
        u16 minute;
        u16 second;
        u16 millisecond;
    };
    
    // TODO(crazy): Implement
    struct ClockDuration
    {
        u16 years;
        u16 months;
        u16 days;
        u16 hours;
        u16 minutes;
        u16 seconds;
        u16 milliseconds;
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
    
    //~ System time measurement
    
    // Local time respects timezones and daylight savings
    // Keep this in mind when using system timepoints
    SystemTimepoint SystemTimepointNow(b8 local);
    
    ClockTime ClockTimeNow(b8 local);
    ClockTime ClockTimeFromSystemTimepoint(SystemTimepoint timepoint);
    
    // TODO(crazy): Fix it
    // ClockDuration ClockDurationFromTo(SystemTimepoint from, SystemTimepoint to);
};


#endif //TOOL_TEMPORAL_H
