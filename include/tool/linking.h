#ifndef _TOOL_LINKING_H
#define _TOOL_LINKING_H

#include "basics.h"
#include "text.h"

//~ Dynamic libraries
//
// Loads shared libraries (modules) while running and looks up their symbols by name. Names go without prefix or
// extension: ModuleLoad("Engine") tries Engine.dll, then libEngine.dll, through Windows' library search, and
// ./libEngine.so, then ./Engine.so, from the working directory on Unix.



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
