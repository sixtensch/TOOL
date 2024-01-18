
#include "basics.h"
#include "linking.h"
#include "exception.h"
#include "text.h"

//#include <stdio.h>

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
        // Convert the string to Windows-proper wide character UTF16
        StringBuilder builder;
        StringBuilderInit(&builder, 256, StringTypeUTF16);
        StringBuilderAdd(&builder, filename);
        StringBuilderAdd(&builder, UTF16(".dll"));
        
        //printf("Trying to open %ls\n", (wchar_t*)builder.str16); 
        Module module = LoadLibraryW((wchar_t*)builder.str16);
        
        if (module != nullptr)
        {
            StringBuilderDestroy(&builder);
            return module;
        }
        
        StringBuilderReset(&builder);
        StringBuilderAdd(&builder, UTF16("lib"));
        StringBuilderAdd(&builder, filename);
        StringBuilderAdd(&builder, UTF16(".dll"));
        StringBuilderDestroy(&builder);
        
        //printf("Trying to open %ls\n", (wchar_t*)builder.str16); 
        module = LoadLibraryW((wchar_t*)builder.str16);
        
        if (module != nullptr)
        {
            return module;
        }
        
        ExceptWindowsLast();
        return nullptr;
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
        StringBuilder builder;
        StringBuilderInit(&builder, 256, StringTypeUTF8);
        StringBuilderAdd(&builder, "./lib");
        StringBuilderAdd(&builder, filename);
        StringBuilderAdd(&builder, ".so");
        
        //printf("Trying to open %s\n", builder.str8); 
        Module module = dlopen(builder.str8, (lazy ? RTLD_LAZY : RTLD_NOW) | RTLD_LOCAL);
        
        if (module != nullptr)
        {
            StringBuilderDestroy(&builder);
            return module;
        }
        
        StringBuilderReset(&builder);
        StringBuilderAdd(&builder, "./");
        StringBuilderAdd(&builder, filename);
        StringBuilderAdd(&builder, ".so");
        StringBuilderDestroy(&builder);
        
        //printf("Trying to open %s\n", builder.str8); 
        module = dlopen(builder.str8, (lazy ? RTLD_LAZY : RTLD_NOW) | RTLD_LOCAL);
        
        if (module != nullptr)
        {
            return module;
        }
        
        Except(dlerror());
        return nullptr;
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
