
#include <cstdio>

#define SCRIPT(name) \
void name##_cpp(int a, int b); \
__declspec(dllexport) extern "C" void name(int a, int b) { name##_cpp(a, b); } \
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
    
    int __declspec(dllexport) foo(int a, int b)
    {
        return cppfoo(a, b);
    }
    
    int __declspec(dllexport) bar()
    {
        return 10;
    }
}

SCRIPT(another)
{
    printf("Result: %i", a + b);
}