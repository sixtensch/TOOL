#ifndef TOOL_TEMPORAL_H
#define TOOL_TEMPORAL_H

#include "basics.h"



namespace Tool
{
    //- Type definitions
    
    //~ Performance clock time units
    
    // Represents performance counter clocks on Windows, nanoseconds on Linux. Access through utility functions.
    typedef i64 Timepoint;
    
    struct Duration
    {
        i32 seconds;
        i32 nanoseconds
    };
    
    //~ Wall/system clock time units
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
    struct SystemTime
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
    
    //~ Thread
    
    Thread ThreadCreate(ThreadFunction function, void* data);
    
    void ThreadDetach(Thread thread);
    void ThreadJoin(Thread thread);
    b8 ThreadTryJoin(Thread thread); // Value of true indicates successful join and release
    
    //~ Semaphore
    
    Semaphore SemaphoreCreate(u32 value = 0);
    void SemaphoreDestroy(Semaphore semaphore);
    
    void SemaphorePost(Semaphore semaphore);
    void SemaphoreWait(Semaphore semaphore);
    b8 SemaphoreTryWait(Semaphore semaphore); // Value of true indicates successful decrement
    
    //~ Mutex
    
    Mutex MutexCreate();
    void MutexDestroy(Mutex mutex);
    
    void MutexLock(Mutex mutex);
    void MutexUnlock(Mutex mutex);
};


#endif //TOOL_TEMPORAL_H
