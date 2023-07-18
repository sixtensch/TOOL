#ifndef _VARIADIC_H
#define _VARIADIC_H

#include "basics.h"

namespace Tool
{
    //~ Typedefs
    
    struct Variadic
    {
        u8* mem;
        u32 offset;
        u32 count;
    };
    
    //~ Functions
    
    Variadic VariadicStart(void* lastNonVariadic, u32 count = U32_MAX);
    
    template<typename T>
        Variadic VariadicStart(T& lastNonVariadic, u32 count = U32_MAX) { return VariadicStart((void*)&lastNonVariadic, count); }
    
    Variadic VariadicCopy(Variadic* variadic);
    
    const void* VariadicArg(Variadic* variadic, u32 count);
    template<typename T>
        const T& VariadicArg(Variadic* variadic) { return &((T*)VariadicArg(variadic, 1)); }
    
    void VariadicEnd(Variadic* variadic);
}



//~ Global usings

#ifndef TOOL_NO_VARIADIC

using Tool::VariadicStart;

#endif



#endif //_VARIADIC_H
