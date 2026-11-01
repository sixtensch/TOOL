#include "tool.h"
#include "test.h"
#include "tool/format.h"

#if TOOL_WINDOWS
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

using namespace Tool;

// The test library's file, less its extension, in the working directory
#define TEST_LIBRARY_FILE (TOOL_WINDOWS ? "TOOLTestLib" : "libTOOLTestLib")

static void TestLoads(const c8* name)
{
    Module module = ModuleLoad(name);
    TOOL_ASSERT(module != nullptr, "Could not load %s", name);
    TOOL_ASSERT(ModuleGetSymbol(module, "GetValue") != nullptr);
    ModuleUnload(module);
}

void TestLinking()
{
    TestLoads("TOOLTestLib");

    // Past any length a fixed buffer would be sized for
    c8 dots[201];
    for (u32 i = 0; i < 200; i += 2)
    {
        dots[i] = '.';
        dots[i + 1] = '/';
    }
    dots[200] = 0;

    c8 name[512];
    CStr8Format(name, sizeof(name), "{}{}", (const c8*)dots, TEST_LIBRARY_FILE);
    TestLoads(name);

    c8 directory[256];
    TOOL_ASSERT(getcwd(directory, sizeof(directory)) != nullptr);
    CStr8Format(name, sizeof(name), "{}/{}", (const c8*)directory, TEST_LIBRARY_FILE);
    TestLoads(name);

    printf("Linking: passed\n");
}
