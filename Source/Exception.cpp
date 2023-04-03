
#include "Exception.h"

#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>

namespace Tool
{
    void Except()
    {
        Exception e =
        {
            { '\0' }, 0, ExceptionTypeEmpty, 0
        };
        
        CStringCopy(e.str, "Unknown exception", TOOL_EXCEPTION_CAPACITY);
        
        throw e;
    }
    
    void Except(const c8* format, ...)
    {
        Exception e =
        {
            { '\0' }, 0, ExceptionTypeMessage, 0
        };
        
        va_list args;
        va_start(args, format);
        
        vsnprintf(e.str, TOOL_EXCEPTION_CAPACITY, format, args);
        e.size = CStringLength(e.str, TOOL_EXCEPTION_CAPACITY);
        
        va_end(args);
        
        throw e;
    }
    
    /*
    void Except(const c8* string, u64 size)
    {
        Exception e =
        {
            { '\0' }, size, ExceptionTypeMessage, 0
        };
        
        Copy(e.str, string, size);
        
        throw e;
    }
    */
    
    void ExceptWindows(u32 code)
    {
        Exception e =
        {
            { '\0' }, 0, ExceptionTypeWindows, (u64)code
        };
        
        e.size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                nullptr,
                                code,
                                0,
                                e.str,
                                TOOL_EXCEPTION_CAPACITY,
                                nullptr);
        
        throw e;
    }
    
    void ExceptWindowsLast()
    {
        ExceptWindows(GetLastError());
    }
}