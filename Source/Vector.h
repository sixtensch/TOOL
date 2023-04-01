#ifndef _VECTOR_H
#define _VECTOR_H

#include "Basics.h"
#include "Math.h"



//- Definitions

//~ Parametric

#define VEC2_CAST(v, T) (Tool::Vec2<T> { (T)(v).x, (T)(v).y } )
#define VEC3_CAST(v, T) (Tool::Vec3<T> { (T)(v).x, (T)(v).y, (T)(v).z } )
#define VEC4_CAST(v, T) (Tool::Vec4<T> { (T)(v).x, (T)(v).y, (T)(v).z, (T)(v).w } )



namespace Tool
{
    //- Struct Declarations
    
    //~ 2D Vector
    
    template<typename T>
        struct Vec2 
    {
        T x;
        T y;
        
        Vec2<T> operator-() const; // Unary negation
        
		Vec2<T> operator+(const Vec2<T>& b) const; // Addition
		Vec2<T> operator-(const Vec2<T>& b) const; // Subtraction
		Vec2<T> operator*(T b) const; // Scalar multiplication
		Vec2<T> operator/(T b) const; // Scalar division
		T operator*(const Vec2<T>& b) const; // Dot product
        
		Vec2<T>& operator+=(const Vec2<T>& b);
		Vec2<T>& operator-=(const Vec2<T>& b);
		Vec2<T>& operator*=(T b);
		Vec2<T>& operator/=(T b);
        
		bool operator==(const Vec2<T>& b) const; // Equality comparison
		bool operator!=(const Vec2<T>& b) const; // Inequality comparison
    };
    
    //~ 3D Vector
    
    template<typename T>
        struct Vec3
    {
        union
        {
            struct
            {
                T x;
                T y;
                T z;
            };
            
            struct 
            {
                Vec2<T> v2;
                T z;
            };
        };
        
        Vec3<T> operator-() const; // Unary negation
        
		Vec3<T> operator+(const Vec3<T>& b) const; // Addition
		Vec3<T> operator-(const Vec3<T>& b) const; // Subtraction
        Vec3<T> operator%(const Vec3<T>& b) const; // Cross product
		Vec3<T> operator*(T b) const; // Scalar multiplication
		Vec3<T> operator/(T b) const; // Scalar division
		T operator*(const Vec3<T>& b) const; // Dot product
        
		Vec3<T>& operator+=(const Vec3<T>& b);
		Vec3<T>& operator-=(const Vec3<T>& b);
		Vec3<T>& operator*=(T b);
		Vec3<T>& operator/=(T b);
        
		bool operator==(const Vec3<T>& b) const; // Equality comparison
		bool operator!=(const Vec3<T>& b) const; // Inequality comparison
    };
    
    //~ 4D Vector
    
    template<typename T>
        struct Vec4 
    {
        union 
        {
            struct 
            {
                T x;
                T y;
                T z;
                T w;
            };
            
            struct 
            {
                Vec2<T> v2;
                T z;
                T w;
            };
            
            struct 
            {
                Vec3<T> v3;
                T w;
            };
        };
        
        Vec4<T> operator-() const; // Unary negation
        
		Vec4<T> operator+(const Vec4<T>& b) const; // Addition
		Vec4<T> operator-(const Vec4<T>& b) const; // Subtraction
        Vec4<T> operator*(T b) const; // Scalar multiplication
		Vec4<T> operator/(T b) const; // Scalar division
		T operator*(const Vec4<T>& b) const; // Dot product
        
		Vec4<T>& operator+=(const Vec4<T>& b);
		Vec4<T>& operator-=(const Vec4<T>& b);
		Vec4<T>& operator*=(T b);
		Vec4<T>& operator/=(T b);
        
		bool operator==(const Vec4<T>& b) const; // Equality comparison
		bool operator!=(const Vec4<T>& b) const; // Inequality comparison
    };
    
    //~ Acronyms
    
    typedef Vec2<f32> v2; // Vector2
    typedef Vec3<f32> v3; // Vector3
    typedef Vec4<f32> v4; // Vector4
    
    typedef Vec2<f64> v2d; // Vector2 double precision
    typedef Vec3<f64> v3d; // Vector3 double precision
    typedef Vec4<f64> v4d; // Vector4 double precision
    
    typedef Vec2<i32> p2; // Point2
    typedef Vec3<i32> p3; // Point3
    typedef Vec4<i32> p4; // Point4
    
    typedef Vec2<i64> p2u; // Point2 unsigned
    typedef Vec3<i64> p3u; // Point3 unsigned
    typedef Vec4<i64> p4u; // Point4 unsigned
    
    
    
    //- Helper function declarations
    
    //~ 2D Vector
    
    v2 V2Normalize(v2 vector);
    void V2Normalize(v2* target);
    
    f32 V2Length(v2 vector);
    f32 V2LengthSquared(v2 vector);
    
    //~ 3D Vector
    
    v3 V3Normalize(v3 vector);
    void V3Normalize(v3* target);
    
    f32 V3Length(v3 vector);
    f32 V3LengthSquared(v3 vector);
    
    //~ 4D Vector
    
    v4 V4Normalize(v4 vector);
    void V4Normalize(v4* target);
    
    f32 V4Length(v4 vector);
    f32 V4LengthSquared(v4 vector);
    
    
    
    //- Operator overload implementations
    
    //~ 2D Vector
    
    template<typename T>
        inline Vec2<T> Vec2<T>::operator-() const // Unary negation
    {
        return { -x, -y };
    }
    
    template<typename T>
        inline Vec2<T> Vec2<T>::operator+(const Vec2<T>& b) const // Addition 
    {
        return { x + b.x, y + b.y };
    }
    
    template<typename T>
        inline Vec2<T> Vec2<T>::operator-(const Vec2<T>& b) const // Subtraction 
    {
        return { x - b.x, y - b.y };
    }
    
    template<typename T>
        inline Vec2<T> Vec2<T>::operator*(T b) const // Scalar multiplication 
    {
        return { x * b, y * b }; 
    }
    
    template<typename T>
        inline Vec2<T> Vec2<T>::operator/(T b) const // Scalar division 
    {
        return { x / b, y / b };
    }
    
    template<typename T>
        inline T Vec2<T>::operator*(const Vec2<T>& b) const // Dot product 
    {
        return x * b.x + y * b.y;
    }
    
    template<typename T>
        inline Vec2<T>& Vec2<T>::operator+=(const Vec2<T>& b) 
    {
        x += b.x;
        y += b.y;
        
        return *this;
    }
    
    template<typename T>
        inline Vec2<T>& Vec2<T>::operator-=(const Vec2<T>& b) 
    {
        x -= b.x;
        y -= b.y;
        
        return *this;
    }
    
    template<typename T>
        inline Vec2<T>& Vec2<T>::operator*=(T b) 
    {
        x *= b;
        y *= b;
        
        return *this;
    }
    
    
    template<typename T>
        inline Vec2<T>& Vec2<T>::operator/=(T b) 
    {
        x /= b;
        y /= b;
        
        return *this;
    }
    
    template<typename T>
        inline bool Vec2<T>::operator==(const Vec2<T>& b) const // Equality comparison 
    {
        return x == b.x && y == b.y;
    }
    
    template<typename T>
        inline bool Vec2<T>::operator!=(const Vec2<T>& b) const // Inequality comparison 
    {
        return x != b.x || y != b.y;
    }
    
    //~ 3D Vector
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator-() const // Unary negation
    {
        return { -x, -y, -z };
    }
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator+(const Vec3<T>& b) const // Addition 
    {
        return { x + b.x, y + b.y, z + b.z };
    }
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator-(const Vec3<T>& b) const // Subtraction 
    {
        return { x - b.x, y - b.y, z - b.z };
    }
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator%(const Vec3<T>& b) const // Subtraction 
    {
        return 
        { 
            y * b.z - z * b.y,
            z * b.x - x * b.z,
            x * b.y - y * b.x
        };
    }
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator*(T b) const // Scalar multiplication 
    {
        return { x * b, y * b, z * b }; 
    }
    
    template<typename T>
        inline Vec3<T> Vec3<T>::operator/(T b) const // Scalar division 
    {
        return { x / b, y / b, z / b };
    }
    
    template<typename T>
        inline T Vec3<T>::operator*(const Vec3<T>& b) const // Dot product 
    {
        return x * b.x + y * b.y + z * b.z;
    }
    
    template<typename T>
        inline Vec3<T>& Vec3<T>::operator+=(const Vec3<T>& b) 
    {
        x += b.x;
        y += b.y;
        z += b.z;
        
        return *this;
    }
    
    template<typename T>
        inline Vec3<T>& Vec3<T>::operator-=(const Vec3<T>& b) 
    {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        
        return *this;
    }
    
    template<typename T>
        inline Vec3<T>& Vec3<T>::operator*=(T b) 
    {
        x *= b;
        y *= b;
        z *= b;
        
        return *this;
    }
    
    
    template<typename T>
        inline Vec3<T>& Vec3<T>::operator/=(T b) 
    {
        x /= b;
        y /= b;
        z /= b;
        
        return *this;
    }
    
    template<typename T>
        inline bool Vec3<T>::operator==(const Vec3<T>& b) const // Equality comparison 
    {
        return x == b.x && y == b.y && z == b.z;
    }
    
    template<typename T>
        inline bool Vec3<T>::operator!=(const Vec3<T>& b) const // Inequality comparison 
    {
        return x != b.x || y != b.y || z != b.z;
    }
    
    //~ 4D Vector
    
    template<typename T>
        inline Vec4<T> Vec4<T>::operator-() const // Unary negation
    {
        return { -x, -y, -z, -w };
    }
    
    template<typename T>
        inline Vec4<T> Vec4<T>::operator+(const Vec4<T>& b) const // Addition 
    {
        return { x + b.x, y + b.y, z + b.z, w + b.w };
    }
    
    template<typename T>
        inline Vec4<T> Vec4<T>::operator-(const Vec4<T>& b) const // Subtraction 
    {
        return { x - b.x, y - b.y, z - b.z, w - b.w };
    }
    
    template<typename T>
        inline Vec4<T> Vec4<T>::operator*(T b) const // Scalar multiplication 
    {
        return { x * b, y * b, z * b, w * b }; 
    }
    
    template<typename T>
        inline Vec4<T> Vec4<T>::operator/(T b) const // Scalar division 
    {
        return { x / b, y / b, z / b, w / b };
    }
    
    template<typename T>
        inline T Vec4<T>::operator*(const Vec4<T>& b) const // Dot product 
    {
        return x * b.x + y * b.y + z * b.z + w * b.w;
    }
    
    template<typename T>
        inline Vec4<T>& Vec4<T>::operator+=(const Vec4<T>& b) 
    {
        x += b.x;
        y += b.y;
        z += b.z;
        w += b.w;
        
        return *this;
    }
    
    template<typename T>
        inline Vec4<T>& Vec4<T>::operator-=(const Vec4<T>& b) 
    {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        w -= b.w;
        
        return *this;
    }
    
    template<typename T>
        inline Vec4<T>& Vec4<T>::operator*=(T b) 
    {
        x *= b;
        y *= b;
        z *= b;
        w *= b;
        
        return *this;
    }
    
    template<typename T>
        inline Vec4<T>& Vec4<T>::operator/=(T b) 
    {
        x /= b;
        y /= b;
        z /= b;
        w /= b;
        
        return *this;
    }
    
    template<typename T>
        inline bool Vec4<T>::operator==(const Vec4<T>& b) const // Equality comparison 
    {
        return x == b.x && y == b.y && z == b.z && w == b.w;
    }
    
    template<typename T>
        inline bool Vec4<T>::operator!=(const Vec4<T>& b) const // Inequality comparison 
    {
        return x != b.x || y != b.y || z != b.z || w != b.w;
    }
    
    
    
    //- Helper function implementations
    
    //~ 2D Vector
    
    inline v2 V2Normalize(v2 vector)
    {
        return vector * (1.0f / V2Length(vector));
    }
    
    inline void V2Normalize(v2* target)
    {
        *target = V2Normalize(*target);
    }
    
    inline f32 V2Length(v2 vector)
    {
        return FSqrt(vector.x * vector.x + vector.y * vector.y);
    }
    
    inline f32 V2LengthSquared(v2 vector)
    {
        return vector.x * vector.x + vector.y * vector.y;
    }
    
    //~ 3D Vector
    
    inline v3 V3Normalize(v3 vector)
    {
        return vector * (1.0f / V3Length(vector));
    }
    
    inline void V3Normalize(v3* target)
    {
        *target = V3Normalize(*target);
    }
    
    inline f32 V3Length(v3 vector)
    {
        return FSqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    }
    
    inline f32 V3LengthSquared(v3 vector)
    {
        return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
    }
    
    //~ 4D Vector
    
    inline v4 V4Normalize(v4 vector)
    {
        return vector * (1.0f / V4Length(vector));
    }
    
    inline void V4Normalize(v4* target)
    {
        *target = V4Normalize(*target);
    }
    
    inline f32 V4Length(v4 vector)
    {
        return FSqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w);
    }
    
    inline f32 V4LengthSquared(v4 vector)
    {
        return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
    }
    
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::v2; // Vector2
using Tool::v3; // Vector3
using Tool::v4; // Vector4

using Tool::v2d; // Vector2 double precision
using Tool::v3d; // Vector3 double precision
using Tool::v4d; // Vector4 double precision

using Tool::p2; // Point2
using Tool::p3; // Point3
using Tool::p4; // Point4

using Tool::p2u; // Point2 unsigned
using Tool::p3u; // Point3 unsigned
using Tool::p4u; // Point4 unsigned

#endif //TOOL_NO_ACRONYMS 



#endif //_VECTOR_H
