
#include "exception.h"
#include "text.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX
#include <errno.h>
#endif

namespace Tool
{
    void Except()
    {
        
        Exception e =
        {
            .str = { '\0' }, 
            .size = 0, 
            .type = ExceptionTypeEmpty, 
            .dataUnsigned = 0
        };
        
        CStr8Copy(e.str, "Unknown exception", TOOL_EXCEPTION_CAPACITY);
        
        throw e;
    }
    
    void Except(const c8* format, ...)
    {
        
        Exception e =
        {
            .str = { '\0' }, 
            .size = 0, 
            .type = ExceptionTypeMessage, 
            .dataUnsigned = 0
        };
        
        va_list args;
        va_start(args, format);
        
        vsnprintf(e.str, TOOL_EXCEPTION_CAPACITY, format, args);
        e.size = CStr8Size(e.str, TOOL_EXCEPTION_CAPACITY);
        
        va_end(args);
        
        throw e;
    }
    
#ifdef TOOL_WINDOWS
    void ExceptWindows(u32 code)
    {
        Exception e =
        {
            .str = { '\0' }, 
            .size = 0, 
            .type = ExceptionTypeWindows, 
            .dataUnsigned = (u64)code
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
#endif // TOOL_WINDOWS
    
#ifdef TOOL_UNIX
    void ExceptUnix(i32 code)
    {
        Exception e =
        {
            .str = { '\0' }, 
            .size = 0, 
            .type = ExceptionTypeUnix, 
            .dataSigned = (i64)code
        };
        
        c8* string = strerror(code);
        
        CStr8Copy(e.str, string, TOOL_EXCEPTION_CAPACITY);
        
        throw e;
    }
    
    void ExceptErrno()
    {
        ExceptUnix(errno);
    }
#endif // TOOL_UNIX
}
