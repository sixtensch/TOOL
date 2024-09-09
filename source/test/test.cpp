
#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <Windows.h>

using namespace Tool;

int main()
{
    Module module = Tool::ModuleLoad("TOOLTestLib");
    
    ClockTime now1 = ClockTimeNow(false);
    ClockTime now2 = ClockTimeNow(true);
    
    SystemTimepoint timepoint = SystemTimepointNow(false);
    ClockTime now3 = ClockTimeFromSystemTimepoint(timepoint);
    
    Sleep(200);
    
    SystemTimepoint after = SystemTimepointNow(false);
    //ClockDuration difference = ClockDurationFromTo(timepoint, after);
    
    IntFunc getValue = (IntFunc)Tool::ModuleGetSymbol(module, "GetValue");
    VoidFunc resetValue = (VoidFunc)Tool::ModuleGetSymbol(module, "ResetValue");
    VoidFunc callModule = (VoidFunc)Tool::ModuleGetSymbol(module, "CallModule");
    
    resetValue();
    int value = getValue();
    
    callModule();
    value = getValue();
    
    return 0;
}

