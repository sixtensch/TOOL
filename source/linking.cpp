
#include <basics.h>
#include <linking.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#define LOAD_FCN(s) ()
#endif

#ifdef TOOL_LINUX
#include <dlfcn.h>
#endif

namespace Tool
{
    Module ModuleLoad(const c8* filename)
    {
    }
    
    void ModuleUnload(Module module)
    {
        
    }
}