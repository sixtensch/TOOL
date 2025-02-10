
#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <Windows.h>
#include <string>

using namespace Tool;

class A
{
public:
    A() { printf("Parent constructed\n"); }
    virtual ~A() { printf("Parent destructed\n"); }
};

class B : public A
{
public:
    B() { printf("Child constructed\n"); }
    virtual ~B() { printf("Child destructed\n"); }
};

class C
{
public:
    C(A& a) : ma(a) { printf("Utility constructed\n"); }

    A& ma;
};

int main()
{
    Arena arena = {};
    ArenaInit(&arena, 1024);

    std::string* string = ArenaPlace<std::string>(&arena, "Hello there!");
    printf("Value: %s\n", string->c_str());

    A* a = ArenaPlace<B>(&arena);
    C* c = ArenaPlace<C>(&arena, *a);

    a->~A();

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

