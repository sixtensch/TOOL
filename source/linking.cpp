
#include <basics.h>
#include <linking.h>
#include <exception.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_LINUX
#include <dlfcn.h>
#endif

namespace Tool
{
    Module ModuleLoad(const c8* filename, bool lazy)
    {
        Module module = nullptr;
        
#if TOOL_WINDOWS
        
        s16 wstr = S16FromCStr8(filename, ALLOC());
        module = LoadLibraryW((wchar_t*)wstr.str);
        
        if (module == nullptr)
        {
            ExceptWindowsLast();
        }
        
#elif TOOL_LINUX
        
        module = dlopen(filename, (lazy ? RTLD_LAZY : RTLD_NOW) | RTLD_LOCAL);
        
        if (module == nullptr)
        {
            Except(dlerror());
        }
        
#endif
        
        return module;
    }
    
    void ModuleUnload(Module module)
    {
#if TOOL_WINDOWS
        
        if (!FreeLibrary((HMODULE)module))
        {
            ExceptWindowsLast();
        }
        
#elif TOOL_LINUX
        
        if (dlclose(module))
        {
            Except(dlerror());
        }
        
#endif
    }
    
    void* ModuleGetSymbol(Module module, const c8* name)
    {
        void* symbol = nullptr;
        
#if TOOL_WINDOWS
        
        
        
#elif TOOL_LINUX
        
        // Clear previous error state
        dlerror();
        
        symbol = dlsym(module, name);
        
        const c8* error = dlerror();
        if (error != null)
        {
            Except(error);
        }
        
#endif
        
        return symbol;
    }
}