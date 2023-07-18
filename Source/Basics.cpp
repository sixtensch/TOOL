
#include "basics.h"

#include <cstring>
#include <Windows.h>



// TODO(crazy): Extend these to work on Linux and MacOS

namespace Tool
{
    void Copy(void* destination, const void* source, u64 size)
    {
        CopyMemory(destination, source, size);
    }
    
    void Copy(void* destination, const void* source, u64 count, u64 size)
    {
        Copy(destination, source, count * size);
    }
}