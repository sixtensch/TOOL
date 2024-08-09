
#include "tool.h"
#include <stdio.h>
#include <windows.h>
#include <directxmath.h>

using namespace Tool;
using namespace DirectX;

#pragma comment (lib, "d3d12.lib")

//Mutex mutex = MutexCreate();
//Semaphore semaphore = SemaphoreCreate();
//int total = 0;
//
//void foo(void* data)
//{
//SemaphoreWait(semaphore);
//for (i32 i = 0; i < 100000; i++)
//{
//total += 100;
//}
//}
//
//int main()
//{
//const i32 count = 20;
//Thread threads[count];
//
//for (i32 i = 0; i < count; i++)
//threads[i] = ThreadCreate(foo, nullptr);
//
//SemaphorePost(semaphore);
//
//Sleep(100);
//
//SemaphorePost(semaphore);
//SemaphorePost(semaphore);
//SemaphorePost(semaphore);
//
//Sleep(100);
//
//return 0;
//}

int main()
{
    Timepoint before = TimepointNow();
    
    Sleep(1000);
    
    Timepoint after = TimepointNow();
    f32 time = SecondsFromTo<f32>(before, after);
    
    return 0;
}

