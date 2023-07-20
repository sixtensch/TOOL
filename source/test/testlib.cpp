
#define SCRIPT(name) \
static void name##_cpp(int a, int b); \
static __declspec(dllexport) extern "C" void name(int a, int b) { name##function(a, b) } \
static void name##_cpp(int a, int b)

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
    return a * b;
}