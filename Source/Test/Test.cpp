
#include "TOOL.h"

int main()
{
    
    
    m4 a = Tool::M4Scale(1, 2, 3, 4);
    m4 b = Tool::M4Translation(1, 2, 3, 4);
    
    v4 v = { 1, 1, 1, 1 };
    v4 av = a * v;
    v4 bv = b * v;
    
    m3 c = Tool::M3RotationX(PI * 0.1f);
    m3 d = Tool::M3RotationY(PI * 0.1f);
    m3 e = Tool::M3RotationZ(PI * 0.1f);
    
    v3 u = { 1, 0, 0 };
    v3 uc = c * u;
    v3 ud = d * u;
    v3 ue = e * u;
    
    m3 f = d * c * e;
    m3 g = Tool::M3Rotation(PI * 0.1f, PI * 0.1f, PI * 0.1f);
    
    return 0;
}
