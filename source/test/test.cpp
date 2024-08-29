
#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;

int main()
{
    Module module = Tool::ModuleLoad("TOOLTestLib");
    
    IntFunc getValue = (IntFunc)Tool::ModuleGetSymbol(module, "GetValue");
    VoidFunc resetValue = (VoidFunc)Tool::ModuleGetSymbol(module, "ResetValue");
    VoidFunc callModule = (VoidFunc)Tool::ModuleGetSymbol(module, "CallModule");
    
    resetValue();
    int value = getValue();
    
    callModule();
    value = getValue();
    
    return 0;
}

