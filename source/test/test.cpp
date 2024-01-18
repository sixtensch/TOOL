
#include "tool.h"

#include <stdio.h>

typedef int (*foofunc)(int, int);
typedef int (*barfunc)();
typedef void (*anotherfunc)(int, int);
typedef void (*setsfunc)(void (*)());

static int s = 0;

void sets()
{
    s = 10;
}

int main()
{
    Tool::Module testLib;
    
    try
    {
        testLib = Tool::ModuleLoad("TOOLTestLib");
    }
    catch (Tool::Exception e)
    {
        printf("Error: %s\n", e.str);
    };
    
    foofunc a = (foofunc)Tool::ModuleGetSymbol(testLib, "foo");
    barfunc b = (barfunc)Tool::ModuleGetSymbol(testLib, "bar");
    anotherfunc e = (anotherfunc)Tool::ModuleGetSymbol(testLib, "another");
    setsfunc f = (setsfunc)Tool::ModuleGetSymbol(testLib, "callsets");
    
    int c = a(5, 5);
    int d = b();
    
    printf("c: %i, d: %i\n", c, d);
    
    e(10, 10);
    
    f(sets);
    
    printf("s: %i\n", s);
    
    return 0;
}
