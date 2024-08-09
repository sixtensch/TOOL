#include "temporal.h"



//- Platform-agnostic

//~ Unix
#if defined(TOOL_UNIX)

#include <time.h>
#include <sys/types.h>

//~ Windows
#elif defined(TOOL_WINDOWS)

#include <Windows.h>

#endif



namespace Tool
{
    
    //- Module
    
    //~ Module Windows implementation
    
#if TOOL_WINDOWS
    
    
    
#endif // TOOL_WINDOWS
    
    //~ Module Unix implementation
    
#if TOOL_UNIX
    
    
    
#endif // TOOL_UNIX
    
}
