
#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <string>

#if TOOL_MSVC
#include <crtdbg.h>
#endif

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
#if TOOL_MSVC && defined(_DEBUG)
    // Debug CRT reports print rather than wait on a dialog, so an unattended run fails instead of hanging
    for (int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
    {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif

    TestHashMap();
    TestMathematics();
    TestMatrix();
    TestColor();
    TestRandom();
    TestSort();
    TestThreading();
    TestIO();
    TestNetwork();
    TestText();
    TestFormat();

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
    
#if TOOL_MIRRORED_MEMORY
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

