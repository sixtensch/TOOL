#ifndef _VECTOR_H
#define _VECTOR_H

#include "Basics.h"

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
            Vec2<T> v2;
            
            struct 
            {
                T x;
                T y;
            };
        };
        
        T z;
        
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
            Vec3<T> v3;
            
            struct 
            {
                union 
                {
                    Vec2<T> v2;
                    
                    struct
                    {
                        T x;
                        T y;
                    };
                };
                
                T z;
            };
        };
        
        T w;
        
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
    
    typedef Tool::Vec2<f32> v2; // Vector2
    typedef Tool::Vec3<f32> v3; // Vector3
    typedef Tool::Vec4<f32> v4; // Vector4
    
    typedef Tool::Vec2<f64> v2d; // Vector2 double precision
    typedef Tool::Vec3<f64> v3d; // Vector3 double precision
    typedef Tool::Vec4<f64> v4d; // Vector4 double precision
    
    typedef Tool::Vec2<i32> p2; // Point2
    typedef Tool::Vec3<i32> p3; // Point3
    typedef Tool::Vec4<i32> p4; // Point4
    
    typedef Tool::Vec2<i64> p2u; // Point2 unsigned
    typedef Tool::Vec3<i64> p3u; // Point3 unsigned
    typedef Tool::Vec4<i64> p4u; // Point4 unsigned
    
    
    
    //- Helper function declarations
    
    //~ 2D Vector
    
    // TODO(crazy): Implement
    
    
    //~ 3D Vector
    
    // TODO(crazy): Implement
    
    
    //~ 4D Vector
    
    // TODO(crazy): Implement
    
    
    
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
    
    // TODO(crazy): Implement
    
    //~ 3D Vector
    
    // TODO(crazy): Implement
    
    //~ 4D Vector
    
    // TODO(crazy): Implement
    
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
