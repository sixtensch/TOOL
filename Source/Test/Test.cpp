
#include "TOOL.h"

#include "TOOL/Random.h"
#include "TOOL/Intrinsics.h"

void Destroy()
{
    
}

int main()
{
    float data[4] = { 1.0f, 2.0f, 2.0f, 1.0f };
    
    f32_x4 a = Tool::F32x4Load(data);
    f32_x4 b = Tool::F32x4Set(1.0f, 3.0f, 1.0f, 1.0f);
    f32_x4 c = Tool::F32x4AddSub(a, b);
    Tool::F32x4Store(c, data);
    
    f32_x4 d = { _mm_cmp_ps(a.m, b.m, Tool::ComparisonEquals) };
    
    return (int)c.f[0];
}
