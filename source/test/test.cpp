
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
    File file = 0;
    b8 success = Tool::FileOpen(&file, "Testfile.txt", OpenModeNewOrAppend); 
    Tool::FileWrite(file, "Hello world!", sizeof("Hello world!") - 1);
    Tool::FileClose(file);
    
    return 0;
}

