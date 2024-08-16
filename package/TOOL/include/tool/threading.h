#ifndef THREADING_H
#define THREADING_H

#include "basics.h"



namespace Tool
{
    //- Type definitions
    
    //~ Threading
    
    typedef u64 Thread;
    typedef u64 Semaphore;
    typedef u64 Mutex;
    
    // TODO(crazy): Implement
    typedef u64 Barrier;
    
    // TODO(crazy): Implement
    struct ThreadArray
    {
        Thread* threads;
        i32 count;
    };
    
    // TODO(crazy): Implement
    struct ThreadPool
    {
        struct Job
        { 
            void* data;
            b8 active;
        };
        
        Thread* threads;
        i32 count;
        
        Job* jobQueue;
        i32 jobReadHead;
        i32 jobWriteHead;
        i32 jobCapacity;
        
        Semaphore semaphore;
    };
    
    typedef void (*ThreadFunction)(void* data);
    typedef void (*ThreadArrayFunction)(void* data, i32 index);
    typedef void (*ThreadPoolFunction)(void* job);
    
    
    
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

#endif //THREADING_H
