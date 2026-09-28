#include "error.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX
#include <errno.h>
#endif



//~ State

static thread_local Tool::Error errorRecord = {};
static thread_local Tool::c8 errorFormatted[TOOL_ERROR_MESSAGE_CAPACITY] = {};

static Tool::ErrorHandler errorHandler = nullptr;
static Tool::AssertHandler assertHandler = nullptr;



//~ Static helper functions

static Tool::b8 ErrorRecord(Tool::ErrorKind kind, Tool::i64 code, Tool::ErrorSite site)
{
    errorRecord.kind = kind;
    errorRecord.code = code;
    errorRecord.site = site;
    
    if (errorHandler != nullptr)
    {
        errorHandler(&errorRecord);
    }
    
    return false;
}



namespace Tool
{
    //~ Error record
    
    const Error* ErrorLast()
    {
        return &errorRecord;
    }
    
    const c8* ErrorMessage()
    {
        switch (errorRecord.kind)
        {
            case ErrorKindMessage:
            return errorRecord.message;

#ifdef TOOL_WINDOWS
            case ErrorKindWindows:
            {
                DWORD size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                            nullptr,
                                            (DWORD)errorRecord.code,
                                            0,
                                            errorFormatted,
                                            TOOL_ERROR_MESSAGE_CAPACITY,
                                            nullptr);
                
                if (size == 0)
                {
                    snprintf(errorFormatted, TOOL_ERROR_MESSAGE_CAPACITY, "Unknown Windows error");
                    return errorFormatted;
                }
                
                // System messages end in a line break
                while (size > 0 && (errorFormatted[size - 1] == '\n' || errorFormatted[size - 1] == '\r'))
                {
                    errorFormatted[--size] = '\0';
                }
                
                return errorFormatted;
            }
#endif

#ifdef TOOL_UNIX
            case ErrorKindUnix:
            snprintf(errorFormatted, TOOL_ERROR_MESSAGE_CAPACITY, "%s", strerror((i32)errorRecord.code));
            return errorFormatted;
#endif

            default:
            return "";
        }
    }
    
    void ErrorClear()
    {
        errorRecord.kind = ErrorKindNone;
        errorRecord.code = 0;
        errorRecord.site = {};
        errorRecord.message[0] = '\0';
    }
    
    
    
    //~ Handlers
    
    void ErrorSetHandler(ErrorHandler handler)
    {
        errorHandler = handler;
    }
    
    void AssertSetHandler(AssertHandler handler)
    {
        assertHandler = handler;
    }
    
    
    
    //~ Macro backends
    
    b8 ErrorFail(ErrorSite site, const c8* format, ...)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(errorRecord.message, TOOL_ERROR_MESSAGE_CAPACITY, format, args);
        va_end(args);
        
        return ErrorRecord(ErrorKindMessage, 0, site);
    }

#ifdef TOOL_WINDOWS
    b8 ErrorFailWindows(ErrorSite site, u32 code)
    {
        errorRecord.message[0] = '\0';
        return ErrorRecord(ErrorKindWindows, (i64)code, site);
    }
    
    b8 ErrorFailWindowsLast(ErrorSite site)
    {
        return ErrorFailWindows(site, (u32)GetLastError());
    }
#endif

#ifdef TOOL_UNIX
    b8 ErrorFailErrno(ErrorSite site)
    {
        errorRecord.message[0] = '\0';
        return ErrorRecord(ErrorKindUnix, (i64)errno, site);
    }
#endif

    void AssertFail(const c8* expression, ErrorSite site, const c8* format, ...)
    {
        // Formatted on the stack, so an assert raised while the error record is in use leaves it intact
        c8 message[TOOL_ERROR_MESSAGE_CAPACITY] = { '\0' };
        
        if (format != nullptr)
        {
            va_list args;
            va_start(args, format);
            vsnprintf(message, TOOL_ERROR_MESSAGE_CAPACITY, format, args);
            va_end(args);
        }
        
        if (assertHandler != nullptr)
        {
            assertHandler(expression, &site, message);
        }
        else
        {
            fprintf(stderr, "%s: %s%s%s\n    at %s (%s:%u)\n",
                    expression != nullptr ? "ASSERT" : "PANIC",
                    expression != nullptr ? expression : "",
                    (expression != nullptr && message[0] != '\0') ? " - " : "",
                    message,
                    site.function, site.file, site.line);
            fflush(stderr);
        }

#ifdef TOOL_WINDOWS
        if (IsDebuggerPresent())
        {
            __debugbreak();
        }
        
        // The CRT's debug "abort() has been called" dialog would block the process after the report above
        _set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif

        abort();
    }
}
