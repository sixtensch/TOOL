
#include "Basics.h"

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
    
    u64 CStringLength(const c8* string, u64 capacity)
    {
        u64 size = 0;
        for (u64 i = 0; i < capacity - 1; i++)
        {
            size = i;
            if (string[i] == '\0')
            {
                break;
            }
        }
        
        return size;
    }
    
    u64 CStringCopy(c8* destination, const c8* source, u64 capacity)
    {
        u64 size = CStringLength(source);
        Copy((void*)destination, (const void*)source, size);
        return size;
    }
}