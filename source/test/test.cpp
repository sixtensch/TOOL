
#include "tool.h"

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
    Tool::Module testLib = Tool::ModuleLoad("TOOLTestLib.dll");
    
    foofunc a = (foofunc)Tool::ModuleGetSymbol(testLib, "foo");
    barfunc b = (barfunc)Tool::ModuleGetSymbol(testLib, "bar");
    anotherfunc e = (anotherfunc)Tool::ModuleGetSymbol(testLib, "another");
    setsfunc f = (setsfunc)Tool::ModuleGetSymbol(testLib, "callsets");
    
    int c = a(5, 5);
    int d = b();
    
    e(10, 10);
    
    f(sets);
    
    return 0;
}
