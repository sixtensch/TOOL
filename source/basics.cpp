
#include "basics.h"

#include <cstring>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX

#endif



// TODO(crazy): Extend these to work on Linux and MacOS

namespace Tool
{
    void Copy(void* destination, const void* source, u64 size)
    {
#ifdef TOOL_WINDOWS
        CopyMemory(destination, source, size);
#endif
        
#ifdef TOOL_UNIX
        memcpy(destination, source, size);
#endif
    }
    
    void Copy(void* destination, const void* source, u64 count, u64 size)
    {
        Copy(destination, source, count * size);
    }
}
