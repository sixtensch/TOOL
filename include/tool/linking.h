#ifndef _LINKING_H
#define _LINKING_H

#include "basics.h"
#include "text.h"

namespace Tool
{
    
    //- Type definitions
    
    //~ Module
    // Modules are dynamic libraries loaded during runtime. Function pointers can be fetched from these modules
    // after loading.
    
    typedef void* Module;
    
    
    
    //- Helper functions
    
    //~ Dynamic library run-time functions
    
    // Open and load a dynamic library (module).
    Module ModuleLoad(const c8* filename, bool lazy = true);
    
    // Unload a previously loaded module.
    void ModuleUnload(Module module);
    
    // Retrieve a symbol pointer from a loaded module.
    // These can represent either function pointers or variables.
    void* ModuleGetSymbol(Module module, const c8* name);
    
    // Retrieve a variable pointer from a loaded module.
    template<typename T>
        inline T* ModuleGetVariable(Module module, const c8* name) { return (T*)ModuleGetSymbol(module, name); }
    
    // Retrieve a functin pointer from a loaded module.
    //template<typename T_return, typename... T_params>
    //inline T_return (*ModuleGetFunction(Module module, const c8* name))(int, int) { return (T_return (*)(T_params...))ModuleGetSymbol(module, name); }
    
}

#endif //_LINKING_H
