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
    
    Module ModuleLoad(const c8* filename);
    void ModuleUnload(Module module);
}

#endif //_LINKING_H
