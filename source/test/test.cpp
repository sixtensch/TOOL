
#include "tool.h"
#include "test.h"
#include <stdio.h>
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
    TestHashMap();
    TestMathematics();
    TestMatrix();
    TestColor();
    TestRandom();
    TestThreading();
    TestIO();
    TestText();

    const c8* intrinsicsFailure = nullptr;
    u32 intrinsicsFailures = TestIntrinsicsRun(&intrinsicsFailure);
    printf("Intrinsics: %u failures%s%s\n", intrinsicsFailures, intrinsicsFailures ? ", first in " : "", intrinsicsFailures ? intrinsicsFailure : "");

    Arena arena = {};
    ArenaInit(&arena, Allocator(), 1024, "Test");
    
    std::string* string = ArenaPlace<std::string>(&arena, "Hello there!");
    printf("Value: %s\n", string->c_str());
    
    A* a = ArenaPlace<B>(&arena);
    C* c = ArenaPlace<C>(&arena, *a);
    
    a->~A();

    {
        printf("Start of scope\n");
        TOOL_DEFER(printf("End of scope 1\n"));
        TOOL_DEFER(printf("End of scope 2\n"));
        TOOL_DEFER(printf("End of scope 4\n"));
        TOOL_DEFER(printf("End of scope 5\n"));
        printf("Part of scope\n");
    }
    
#ifdef TOOL_MIRRORED_MEMORY
    MemoryLoop loop;
    Tool::LoopAlloc(&loop, 1, 1);
    
    char* contents = (char*)loop.start;
    contents[0] = 'A';
    
    char result = contents[1 << 16];
#endif
    
    Module module = Tool::ModuleLoad("TOOLTestLib");
    
    ClockTime now1 = ClockTimeNow(false);
    ClockTime now2 = ClockTimeNow(true);
    
    SystemTimepoint timepoint = SystemTimepointNow(false);
    ClockTime now3 = ClockTimeFromSystemTimepoint(timepoint);
    
    IntFunc getValue = (IntFunc)Tool::ModuleGetSymbol(module, "GetValue");
    VoidFunc resetValue = (VoidFunc)Tool::ModuleGetSymbol(module, "ResetValue");
    VoidFunc callModule = (VoidFunc)Tool::ModuleGetSymbol(module, "CallModule");
    
    resetValue();
    int value = getValue();
    
    callModule();
    value = getValue();
    
    return 0;
}

