#ifndef _TOOL_EXCEPTION_H
#define _TOOL_EXCEPTION_H

#include "basics.h"



//~ Definitions

#define TOOL_EXCEPTION_CAPACITY 8096



namespace Tool
{
    //~ Definitions
    
    enum ExceptionType
    {
        ExceptionTypeEmpty,   // Data is nothing
        ExceptionTypeMessage, // Data is nothing
        ExceptionTypeWindows, // Data is an unsigned integer Windows error code
        ExceptionTypeUnix     // Data is a signed integer Unix error code (potentially errno)
    };
    
    struct Exception
    {
        c8 str[TOOL_EXCEPTION_CAPACITY];
        u64 size;
        
        ExceptionType type;
        
        union
        {
            u64 dataUnsigned;
            i64 dataSigned;
        };
    };
    
    
    
    //~ Helper functions
    
    void Except();
    void Except(const c8* format, ...);
    
#ifdef TOOL_WINDOWS
    void ExceptWindows(u32 code);
    void ExceptWindowsLast();
#endif
    
#ifdef TOOL_UNIX
    void ExceptUnix(i32 code);
    void ExceptErrno();
#endif
}



#endif //_EXCEPTION_H
