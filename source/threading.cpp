#include "threading.h"
#include "exception.h"



//- Platform-agnostic

//~ Unix
#if defined(TOOL_UNIX)

#include <sys/types.h>
#define TOOL_THREAD_T pthread_t

//~ Windows
#elif defined(TOOL_WINDOWS)

#include <Windows.h>
#define TOOL_THREAD_T HANDLE

#define CIRCULAR_COUNT 256

struct HiddenThreadParams
{
    void* function;
    void* data;
    i32 index;
    b8 block;
};

static Tool::Mutex circularMutex = Tool::MutexCreate();
static i32 circularIndex = 0;
static HiddenThreadParams circularThreadParams[CIRCULAR_COUNT] = {};

static DWORD ThreadIndirection(LPVOID parameter)
{
    HiddenThreadParams* paramsPtr = (HiddenThreadParams*)parameter;
    HiddenThreadParams params = *paramsPtr;
    paramsPtr->block = false;
    
    if (params.index == -1)
    {
        Tool::ThreadFunction function = (Tool::ThreadFunction)params.function;
        function(params.data);
    }
    else
    {
        Tool::ThreadArrayFunction function = (Tool::ThreadArrayFunction)params.function;
        function(params.data, params.index);
    }
    
    return 0;
}

#endif



namespace Tool
{
    //- Thread
    
    //~ Thread Windows implementation
    
#if defined(TOOL_WINDOWS)
    
    Thread ThreadCreate(ThreadFunction function, void* data)
    {
        // Get and block a thread parameter slot
        MutexLock(circularMutex);
        i32 index = circularIndex;
        while (circularThreadParams[circularIndex].block == true);
        circularThreadParams[index].block = true;
        circularIndex = (circularIndex + 1) % CIRCULAR_COUNT;
        MutexUnlock(circularMutex);
        
        // Populate thread parameters
        HiddenThreadParams* params = &circularThreadParams[index];
        params->function = (void*)function;
        params->data = data;
        params->index = -1;
        
        // Create the thread
        HANDLE handle = CreateThread(nullptr, 0, ThreadIndirection, (LPVOID)params, 0, nullptr);
        if (handle == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return *((Thread*)&handle);
    }
    
    void ThreadDetach(Thread thread)
    {
        HANDLE handle = *((HANDLE*)&thread);
        CloseHandle(handle);
    }
    
    void ThreadJoin(Thread thread)
    {
        HANDLE handle = *((HANDLE*)&thread);
        
        DWORD result = WaitForSingleObject(handle, INFINITE);
        if (result == WAIT_FAILED)
        {
            ExceptWindowsLast();
        }
        
        CloseHandle(handle);
    }
    
    b8 ThreadTryJoin(Thread thread)
    {
        HANDLE handle = *((HANDLE*)&thread);
        
        DWORD result = WaitForSingleObject(handle, 0);
        switch (result)
        {
            case WAIT_OBJECT_0:
            CloseHandle(handle);
            return true;
            
            case WAIT_FAILED:
            ExceptWindowsLast();
            return false;
            
            default:
            return false;
        }
    }
    
#endif
    
    //- Semaphore
    
    //~ Semaphore Windows implementation
    
#if defined(TOOL_WINDOWS)
    
    Semaphore SemaphoreCreate(u32 value)
    {
        HANDLE handle = CreateSemaphore(nullptr, (LONG)value, I32_MAX, nullptr);
        if (handle == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return *((Semaphore*)&handle);
    }
    
    void SemaphoreDestroy(Semaphore semaphore)
    {
        HANDLE handle = *((HANDLE*)&semaphore);
        CloseHandle(handle);
    }
    
    void SemaphorePost(Semaphore semaphore)
    {
        HANDLE handle = *((HANDLE*)&semaphore);
        b32 result = ReleaseSemaphore(handle, 1, nullptr);
        if (result == false)
        {
            ExceptWindowsLast();
        }
    }
    
    void SemaphoreWait(Semaphore semaphore)
    {
        HANDLE handle = *((HANDLE*)&semaphore);
        DWORD result = WaitForSingleObject(handle, INFINITE);
        if (result == WAIT_FAILED)
        {
            ExceptWindowsLast();
        }
    }
    
    b8 SemaphoreTryWait(Semaphore semaphore) // Value of true indicates successful decrement
    {
        HANDLE handle = *((HANDLE*)&semaphore);
        DWORD result = WaitForSingleObject(handle, 0);
        switch (result)
        {
            case WAIT_OBJECT_0:
            return true;
            
            case WAIT_FAILED:
            ExceptWindowsLast();
            return false;
            
            default:
            return false;
        }
    }
    
#endif
    
    //- Mutex
    
    //~ Mutex Windows implementation
    
#if defined(TOOL_WINDOWS)
    
    Mutex MutexCreate()
    {
        HANDLE handle = CreateMutex(nullptr, false, nullptr);
        if (handle == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return *((Mutex*)&handle);
    }
    
    void MutexDestroy(Mutex mutex)
    {
        HANDLE handle = *((HANDLE*)&mutex);
        CloseHandle(handle);
    }
    
    void MutexLock(Mutex mutex)
    {
        HANDLE handle = *((HANDLE*)&mutex);
        DWORD result = WaitForSingleObject(handle, INFINITE);
        if (result == WAIT_FAILED)
        {
            ExceptWindowsLast();
        }
    }
    
    void MutexUnlock(Mutex mutex)
    {
        HANDLE handle = *((HANDLE*)&mutex);
        b32 result = ReleaseMutex(handle);
        if (result == false)
        {
            ExceptWindowsLast();
        }
    }
    
#endif
    
}

