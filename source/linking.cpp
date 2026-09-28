
#include "basics.h"
#include "linking.h"
#include "error.h"
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
        
        // A file that exists but fails to load (e.g. a bad image) is the error worth reporting
        DWORD error = GetLastError();
        
        StringBuilderReset(&builder);
        StringBuilderAdd(&builder, UTF16("lib"));
        StringBuilderAdd(&builder, filename);
        StringBuilderAdd(&builder, UTF16(".dll"));
        
        //printf("Trying to open %ls\n", (wchar_t*)builder.str16);
        module = LoadLibraryW((wchar_t*)builder.str16);
        
        if (module == nullptr && error == ERROR_MOD_NOT_FOUND)
        {
            error = GetLastError();
        }
        
        StringBuilderDestroy(&builder);
        
        if (module != nullptr)
        {
            return module;
        }
        
        TOOL_FAIL_WINDOWS_CODE(error);
        return nullptr;
    }
    
    void ModuleUnload(Module module)
    {
        b32 result = FreeLibrary((HMODULE)module);
        TOOL_ASSERT(result, "Could not unload Module (Windows error %lu)", GetLastError());
    }
    
    void* ModuleGetSymbol(Module module, const c8* name)
    {
        void* symbol = (void*)GetProcAddress((HMODULE)module, name);
        
        if (symbol == nullptr)
        {
            TOOL_FAIL_WINDOWS();
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
        
        //printf("Trying to open %s\n", builder.str8); 
        module = dlopen(builder.str8, (lazy ? RTLD_LAZY : RTLD_NOW) | RTLD_LOCAL);
        
        StringBuilderDestroy(&builder);
        
        if (module != nullptr)
        {
            return module;
        }
        
        TOOL_FAIL("%s", dlerror());
        return nullptr;
    }
    
    void ModuleUnload(Module module)
    {
        i32 result = dlclose(module);
        TOOL_ASSERT(result == 0, "Could not unload Module: %s", dlerror());
    }
    
    void* ModuleGetSymbol(Module module, const c8* name)
    {
        // Clear previous error state
        dlerror();
        
        void* symbol = dlsym(module, name);
        
        const c8* error = dlerror();
        if (error != nullptr)
        {
            TOOL_FAIL("%s", error);
            return nullptr;
        }
        
        return symbol;
    }
    
#endif // TOOL_UNIX
    
}
