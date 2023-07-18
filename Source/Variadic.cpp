
#include "variadic.h"

#include <cstdarg>

namespace Tool
{
    Variadic VariadicStart(void* lastNonVariadic, u32 count)
    {
        return 
        { 
            (u8*)lastNonVariadic,
            1u,
            count
        };
    }
    
    
    Variadic VariadicCopy(Variadic* variadic)
    {
        return *variadic;
    }
    
    const void* VariadicArg(Variadic* variadic, u32 count)
    {
        u32 offset = variadic->offset;
        variadic->offset += 1;
        
        return (void*)(variadic->mem + offset * (u32)sizeof(u64));
    }
    
    void VariadicEnd(Variadic* variadic)
    {
        variadic->mem = nullptr;
        variadic->offset = 0u;
        variadic->count = 0u;
    }
}
