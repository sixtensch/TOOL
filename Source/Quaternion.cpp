#include "Quaternion.h"
#include "Math.h"

//~ Static functions

static m4 GetMatrix(q4 q, f32 s)
{
    f32 x2 = q.x * q.x;
	f32 xy = q.x * q.y;
	f32 xz = q.x * q.z;
	f32 xw = q.x * q.w;
	f32 y2 = q.y * q.y;
	f32 yz = q.y * q.z;
	f32 yw = q.y * q.w;
	f32 z2 = q.z * q.z;
	f32 zw = q.z * q.w;
    
	return 
    {
        1.0f - s * (y2 + z2), s * (xy - zw),		s * (xz + yw),		0.0f,
        s * (xy + zw),		1.0f - s * (x2 + z2), s * (yz - xw),		0.0f,
        s * (xz - yw),		s * (yz + xw),		1.0f - s * (x2 + y2), 0.0f,
        0.0f,				 0.0f,				 0.0f,				 1.0f
    };
}

namespace Tool
{
    //~ Operators
    
    v3 Quaternion::operator*(v3 vector)
    {
        q4 q = { vector.x, vector.y, vector.z, 1.0f };
        q4 c = Q4Conjugate(*this);
        return (*this * q * c).im;
    }
    
    v4 Quaternion::operator*(v4 vector)
    {
        q4 q = { vector.x, vector.y, vector.z, vector.w };
        q4 c = Q4Conjugate(*this);
        return (*this * q * c).vec;
    }
    
    q4 Quaternion::operator*(f32 scalar)
    {
        return { x * scalar, y * scalar, z * scalar, w * scalar };
    }
    
    q4 Quaternion::operator*(q4 b)
    {
        v3 imaginary = im % b.im + im * b.w + b.im * w;
        
        return 
        {
            imaginary.x,
            imaginary.y,
            imaginary.z,
            w * b.w - im * b.im
        };
    }
    
    q4 Quaternion::operator+(q4 b)
    {
        return
        {
            x + b.x,
            y + b.y,
            z + b.z,
            w + b.w
        };
    }
    
    //~ Initializers
    
    q4 Q4Identity()
    {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }
    
    q4 Q4FromComponents(v4 vector)
    {
        return { vector.x, vector.y, vector.z, vector.w };
    }
    
    q4 Q4FromComponents(v3 imaginary, f32 real)
    {
        return { imaginary.x, imaginary.y, imaginary.z, real };
    }
    
    q4 Q4FromComponents(f32 x, f32 y, f32 z, f32 w)
    {
        return { x, y, z, w };
    }
    
    q4 Q4Axis(v3 normalizedAxis, f32 radians)
    {
        f32 r = radians * 0.5f;
        return Q4FromComponents(normalizedAxis * sinf(r), cosf(r));
    }
    
    q4 Q4Euler(v3 euler)
    {
        return Q4Euler(euler.x, euler.y, euler.z);
    }
    
    q4 Q4Euler(f32 pitch, f32 yaw, f32 roll)
    {
        f32 sx = sinf(pitch * 0.5f);
        f32 sy = sinf(yaw * 0.5f);
        f32 sz = sinf(roll * 0.5f);
        f32 cx = cosf(pitch * 0.5f);
        f32 cy = cosf(yaw * 0.5f);
        f32 cz = cosf(roll * 0.5f);
        
        return
        {
            cx * sy * sz + sx * cy * cz,
            cx * sy * cz - sx * cy * sz,
            sx * sy * cz + cx * cy * sz,
            cx * cy * cz - sx * sy * sz
        };
    }
    
    q4 Q4FromTo(v3 from, v3 to)
    {
        f32 comp = sqrtf(2.0f + from * to);
        return Q4FromComponents((from % to) * (1.0f / comp), comp * 0.5f);
    }
    
    q4 Q4Direction(v3 direction, v3 up)
    {
        up = V3Normalize(up - direction * (up * direction));
        
        v3 right = up % direction;
        
        Quaternion q = { 0 };
        q.w = 0.5f * sqrtf(1.0f + right.x + up.y + direction.z);
        
        f32 rec = 0.25f / q.w;
        
        q.x = rec * (up.z - direction.y);
        q.y = rec * (direction.x - right.z);
        q.z = rec * (right.y - up.x);
        
        return q;
    }
    
    q4 Q4Deconstruct(const m4& matrix)
    {
        f32 w = 0.5f * sqrtf(M4Trace(matrix)); 
        f32 inv = 0.25f / w;
        
        return 
        {
            (matrix.m12 - matrix.m21) * inv,
            (matrix.m20 - matrix.m02) * inv,
            (matrix.m01 - matrix.m12) * inv,
            w
        };
    }
    
    q4 Q4Slerp(q4 from, q4 to, f32 t)
    {
        f32 dot = from.vec * to.vec;
        f32 theta = acosf(dot);
        f32 invSine = 1.0f / sinf(theta);
        return from * (sinf(theta * (1 - t)) * invSine) + to * (sinf(theta * t) * invSine);
    }
    
    //~ Conversions
    
    m4 Q4GetMatrix(q4 quaternion)
    {
        return GetMatrix(quaternion, 2.0f);
    }
    
    m4 Q4GetMatrixNonNormal(q4 quaternion)
    {
        f32 n = Q4Norm(quaternion);
        f32 s = 2.0f / (n * n);
        return GetMatrix(quaternion, s);
    }
    
    //~ Operations
    
    f32 Q4Norm(q4 quaternion)
    {
        return V4Length(quaternion.vec);
    }
    
    q4 Q4Conjugate(q4 quaternion)
    {
        return { -quaternion.x, -quaternion.y, -quaternion.z, quaternion.w };
    }
    
    q4 Q4Normalize(q4 quaternion)
    {
        f32 in = 1.0f / Q4Norm(quaternion);
        return { quaternion.x * in, quaternion.y * in, quaternion.z * in, quaternion.w * in };
    }
    
    q4 Q4Inverse(q4 quaternion)
    {
        f32 n = Q4Norm(quaternion);
        f32 s = -1.0f / (n * n);
        return { quaternion.x * s, quaternion.y * s, quaternion.z * s, quaternion.w * -s };
    }
    
    void Q4Conjugate(q4* target)
    {
        *target = Q4Conjugate(*target);
    }
    
    void Q4Normalize(q4* target)
    {
        *target = Q4Normalize(*target);
    }
    
    void Q4Inverse(q4* target)
    {
        *target = Q4Inverse(*target);
    }
}
