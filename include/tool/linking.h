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
    
}

#endif //_LINKING_H
