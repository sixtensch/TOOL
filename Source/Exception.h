#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include "Basics.h"



//~ Definitions

#define TOOL_EXCEPTION_CAPACITY 8096



namespace Tool
{
    //~ Definitions
    
    enum ExceptionType
    {
        ExceptionTypeEmpty,   // Data is nothing
        ExceptionTypeMessage, // Data is nothing
        ExceptionTypeWindows  // Data is a Windows error code
    };
    
    struct Exception
    {
        c8 str[TOOL_EXCEPTION_CAPACITY];
        u64 size;
        
        ExceptionType type;
        u64 data;
    };
    
    
    
    //~ Helper functions
    
    void Except();
    void Except(const c8* format, ...);
    void ExceptWindows(u32 code);
    void ExceptWindowsLast();
}



#endif //_EXCEPTION_H
