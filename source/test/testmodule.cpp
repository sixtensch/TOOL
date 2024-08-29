
#include "test.h"

DLL_IMPORT void Add3();
DLL_IMPORT void Add5();

int main()
{
    return 0;
}

DLL_EXPORT void CallAdd()
{
    Add3();
    Add5();
}
