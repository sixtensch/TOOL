
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
    m4 matrix = M4ProjectionPerspective(90.0f, 1.0f, 1, 3, ClipTypeDX);
    XMMATRIX xmatrix = XMMatrixPerspectiveFovLH(PI * 0.5f, 1.0f, 1, 3);
    
    v4 a0 = matrix * v4{ 0, 0, 1, 1 };
    v4 b0 = matrix * v4{ 0, 0, 2, 1 };
    v4 c0 = matrix * v4{ 0, 0, 3, 1 };
    
    v4 a1i = v4{ 1, 0, 1, 1 };
    v4 b1i = v4{ 1, 0, 2, 1 };
    v4 c1i = v4{ 1, 0, 3, 1 };
    
    v4 a1 = matrix * a1i;
    v4 b1 = matrix * b1i;
    v4 c1 = matrix * c1i;
    
    XMVECTOR a1x = XMVector4Transform(XMLoadFloat4((XMFLOAT4*)&a1i), xmatrix);
    XMVECTOR b1x = XMVector4Transform(XMLoadFloat4((XMFLOAT4*)&b1i), xmatrix);
    XMVECTOR c1x = XMVector4Transform(XMLoadFloat4((XMFLOAT4*)&c1i), xmatrix);
    
    v4 a2 = matrix * v4{ 0, 1, 1, 1 };
    v4 b2 = matrix * v4{ 0, 1, 2, 1 };
    v4 c2 = matrix * v4{ 0, 1, 3, 1 };
    
    return 0;
}

