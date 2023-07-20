
#include "tool.h"

typedef int (*foofunc)(int, int);
typedef int (*barfunc)();

int main()
{
    Tool::Module testLib = Tool::ModuleLoad("TOOLTestLib.dll");
    
    foofunc a = (foofunc)Tool::ModuleGetSymbol(testLib, "foo");
    barfunc b = (barfunc)Tool::ModuleGetSymbol(testLib, "bar");
    
    int c = a(5, 5);
    int d = b();
    
    return 0;
}
