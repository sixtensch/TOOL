
#include "TOOL.h"

int main()
{
    v2 a = { 0.0f, 1.0f };
    v2 b = { 2.0f, 3.0f };
    
    f32 c = 1.0f;
    
    a = a / c;
    
    return (i32)a.x;
}
