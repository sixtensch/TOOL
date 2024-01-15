
#include <cstdio>

#ifdef TOOL_WINDOWS
#define DLL_EXPORT __declspec(dllexport)
#endif

#ifdef TOOL_UNIX
#define DLL_EXPORT __attribute__((visibility("default")))
#endif

#define SCRIPT(name) \
void name##_cpp(int a, int b); \
DLL_EXPORT extern "C" void name(int a, int b) { name##_cpp(a, b); } \
void name##_cpp(int a, int b)


int main()
{
    return 0;
}

int cppfoo(int a, int b)
{
    return a + b;
}

extern "C"
{
    
    int DLL_EXPORT foo(int a, int b)
    {
        return cppfoo(a, b);
    }
    
    int DLL_EXPORT bar()
    {
        return 10;
    }
    
    void DLL_EXPORT callsets(void (*sets)())
    {
        sets();
    }
}

SCRIPT(another)
{
    printf("Result: %i", a + b);
}