#ifndef _TOOL_ERROR_H
#define _TOOL_ERROR_H

#include "basics.h"



//~ Errors and asserts
//
// Two mechanisms, for two kinds of problem:
// - Failures are expected at runtime, like a missing file. The failing function records what went wrong in the
//   calling thread's error record and returns false or null, and ErrorMessage reads the record afterwards. A
//   handler set with ErrorSetHandler sees each failure as it happens, for logging.
// - Asserts catch bugs: conditions that should never be false. A failed assert calls the assert handler, breaks
//   into an attached debugger on Windows, then aborts.
//
// Nothing here throws.



//~ Definitions

#define TOOL_ERROR_MESSAGE_CAPACITY 1024

#define TOOL_ERROR_SITE ::Tool::ErrorSite{ __FILE__, __FUNCTION__, (::Tool::u32)__LINE__ }



//~ Failure macros
//
// Record a failure into this thread's error record, call the error handler if one is set, and evaluate to false.
// Usage: 'if (p == nullptr) return TOOL_FAIL_WINDOWS();', or 'TOOL_FAIL_WINDOWS(); return nullptr;'.

#define TOOL_FAIL(format, ...) ::Tool::ErrorFail(TOOL_ERROR_SITE, format, ##__VA_ARGS__)

#ifdef TOOL_WINDOWS
#define TOOL_FAIL_WINDOWS() ::Tool::ErrorFailWindowsLast(TOOL_ERROR_SITE)
#define TOOL_FAIL_WINDOWS_CODE(code) ::Tool::ErrorFailWindows(TOOL_ERROR_SITE, (::Tool::u32)(code))
#endif

#ifdef TOOL_UNIX
#define TOOL_FAIL_ERRNO() ::Tool::ErrorFailErrno(TOOL_ERROR_SITE)
#endif



//~ Assert macros
//
// A failed assert calls the assert handler if one is set, breaks into an attached debugger (Windows), then aborts.
// TOOL_ASSERT is compiled into every build. TOOL_DEBUG_ASSERT is only active when TOOL_DEBUG_ASSERTS is defined.
// TOOL_PANIC stops unconditionally, for unreachable or unimplemented paths.
// Message arguments are only evaluated when the assert fails.

#define TOOL_ASSERT(condition, ...) \
    do { if (!(condition)) ::Tool::AssertFail(#condition, TOOL_ERROR_SITE, ##__VA_ARGS__); } while (0)

#ifdef TOOL_DEBUG_ASSERTS
#define TOOL_DEBUG_ASSERT(condition, ...) TOOL_ASSERT(condition, ##__VA_ARGS__)
#else
#define TOOL_DEBUG_ASSERT(condition, ...) do { } while (0)
#endif

#define TOOL_PANIC(format, ...) ::Tool::AssertFail(nullptr, TOOL_ERROR_SITE, format, ##__VA_ARGS__)



namespace Tool
{
    //~ Definitions
    
    enum ErrorKind
    {
        ErrorKindNone,    // No failure recorded
        ErrorKindMessage, // Failure described by a formatted message only
        ErrorKindWindows, // Failure carries a Win32 error code
        ErrorKindUnix     // Failure carries an errno value
    };
    
    struct ErrorSite
    {
        const c8* file;
        const c8* function;
        u32 line;
    };
    
    struct Error
    {
        ErrorKind kind;
        i64 code;
        ErrorSite site;
        c8 message[TOOL_ERROR_MESSAGE_CAPACITY];
    };
    
    // Called on every recorded failure, after the record is written.
    typedef void (*ErrorHandler)(const Error* error);
    
    // Called on a failed assert or panic, before the process stops. Cannot resume. 'expression' is null for a panic.
    typedef void (*AssertHandler)(const c8* expression, const ErrorSite* site, const c8* message);
    
    
    
    //~ Error record
    
    // This thread's most recent failure.
    const Error* ErrorLast();
    
    // Text of this thread's most recent failure. OS error codes are formatted on demand.
    const c8* ErrorMessage();
    
    void ErrorClear();
    
    
    
    //~ Handlers
    
    // Optional. With no handler set, failures are recorded silently.
    void ErrorSetHandler(ErrorHandler handler);
    
    // Optional. With no handler set, a failed assert is printed to stderr.
    void AssertSetHandler(AssertHandler handler);
    
    
    
    //~ Macro backends (use the macros above)
    
    b8 ErrorFail(ErrorSite site, const c8* format, ...);

#ifdef TOOL_WINDOWS
    b8 ErrorFailWindows(ErrorSite site, u32 code);
    b8 ErrorFailWindowsLast(ErrorSite site);
#endif

#ifdef TOOL_UNIX
    b8 ErrorFailErrno(ErrorSite site);
#endif

    [[noreturn]] void AssertFail(const c8* expression, ErrorSite site, const c8* format = nullptr, ...);
}



#endif //_TOOL_ERROR_H
