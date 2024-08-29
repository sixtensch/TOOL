
#include "test.h"
#include "tool.h"

static int value;



// Core

int main()
{
    value = 1;
}

Tool::Module GetModule()
{
    static Tool::Module module = nullptr;
    
    if (module == nullptr)
    {
        module = Tool::ModuleLoad("TOOLTestModule");
    }
    
    return module;
}



// For the module

DLL_EXPORT void Add3()
{
    value += 3;
}

DLL_EXPORT void Add5()
{
    value += 5;
}



// For the nugget

DLL_EXPORT void ResetValue()
{
    value = 1;
}

DLL_EXPORT int GetValue()
{
    return value;
}

DLL_EXPORT void CallModule()
{
    VoidFunc func = (VoidFunc)Tool::ModuleGetSymbol(GetModule(), "CallAdd");
    func();
}


