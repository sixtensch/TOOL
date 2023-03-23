
#include "TOOL.h"

int main()
{
    v2 a = { 0.0f, 1.0f };
    v2 b = { 2.0f, 3.0f };
    
    f32 c = 1.0f;
    
    a = a / c;
    
    m2 matrix = { 0, 0, 0, 0 };
    m2 other = { 0, 0, 0, 0 };
    m2 third = matrix * 2;
    other *= matrix;
    
    v2 v = { 0, 0 };
    v2 tr = matrix * v;
    
    matrix = other;
    
    m4 another = Tool::M4Identity();
    
    m2 id = Tool::M2Scale(0.0, 0.0);
    
    bool cmp = matrix != other;
    
    return (i32)id.m00;
}
