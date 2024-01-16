
#include <basics.h>
#include <linking.h>
#include <exception.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#ifdef TOOL_UNIX
#include <dlfcn.h>
#endif

namespace Tool
{
    
    //- Module
    
    //~ Module Windows implementation
    
#if TOOL_WINDOWS
    
    Module ModuleLoad(const c8* filename, bool lazy)
    {
        StringBuilder name;
        StringBuilderInit(&name, 256, StringTypeUTF16);
        
        // Convert the string to Windows-proper wide character UTF16
        s16 wstr = S16FromCStr8(filename, ALLOC());
        
        Module module = LoadLibraryW((wchar_t*)wstr.str);
        
        if (module == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return module;
    }
    
    void ModuleUnload(Module module)
    {
        if (!FreeLibrary((HMODULE)module))
        {
            ExceptWindowsLast();
        }
    }
    
    void* ModuleGetSymbol(Module module, const c8* name)
    {
        void* symbol = (void*)GetProcAddress((HMODULE)module, name);
        
        if (symbol == nullptr)
        {
            ExceptWindowsLast();
        }
        
        return symbol;
    }
    
#endif // TOOL_WINDOWS
    
    //~ Module Unix implementation
    
#if TOOL_UNIX
    
    Module ModuleLoad(const c8* filename, bool lazy)
    {
        Module module = dlopen(filename, (lazy ? RTLD_LAZY : RTLD_NOW) | RTLD_LOCAL);
        
        if (module == nullptr)
        {
            Except(dlerror());
        }
        
        return module;
    }
    
    void ModuleUnload(Module module)
    {
        if (dlclose(module))
        {
            Except(dlerror());
        }
    }
    
    void* ModuleGetSymbol(Module module, const c8* name)
    {
        // Clear previous error state
        dlerror();
        
        void* symbol = dlsym(module, name);
        
        const c8* error = dlerror();
        if (error != nullptr)
        {
            Except(error);
        }
        
        return symbol;
    }
    
#endif // TOOL_UNIX
    
}
