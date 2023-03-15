#ifndef _VECTOR_H
#define _VECTOR_H

#include "Basics.h"

namespace Tool
{
    // Struct declarations
    
    //~ 2D Vector
    
    template<typename T>
        struct Vec2 
    {
        T x;
        T y;
        
        Vec2<T> operator-() const; // Unary negation
        
		Vec2<T> operator+(const Vec2<T>& b) const; // Addition
		Vec2<T> operator-(const Vec2<T>& b) const; // Subtraction
		Vec2<T> operator*(const T& b) const; // Scalar multiplication
		Vec2<T> operator/(const T& b) const; // Scalar division
		T operator*(const Vec2<T>& b) const; // Dot product
        
		Vec2<T>& operator+=(const Vec2<T>& b);
		Vec2<T>& operator-=(const Vec2<T>& b);
		Vec2<T>& operator*=(const T& b);
		Vec2<T>& operator/=(const T& b);
        
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
		Vec3<T> operator*(const T& b) const; // Scalar multiplication
		Vec3<T> operator/(const T& b) const; // Scalar division
        Vec3<T> operator%(const T& b) const; // Cross product
		T operator*(const Vec3<T>& b) const; // Dot product
        
		Vec3<T>& operator+=(const Vec3<T>& b);
		Vec3<T>& operator-=(const Vec3<T>& b);
		Vec3<T>& operator*=(const T& b);
		Vec3<T>& operator/=(const T& b);
        
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
        Vec4<T> operator*(const T& b) const; // Scalar multiplication
		Vec4<T> operator/(const T& b) const; // Scalar division
		T operator*(const Vec4<T>& b) const; // Dot product
        
		Vec4<T>& operator+=(const Vec4<T>& b);
		Vec4<T>& operator-=(const Vec4<T>& b);
		Vec4<T>& operator*=(const T& b);
		Vec4<T>& operator/=(const T& b);
        
		bool operator==(const Vec4<T>& b) const; // Equality comparison
		bool operator!=(const Vec4<T>& b) const; // Inequality comparison
    };
    
    
    
    // Helper function declarations
    
    //~ 2D Vector
    
    // TODO(crazy): Implement
    
    
    //~ 3D Vector
    
    // TODO(crazy): Implement
    
    
    //~ 4D Vector
    
    // TODO(crazy): Implement
    
    
    
    // Operator overload implementations
    
    //~ 2D Vector
    
    // TODO(crazy): Implement
    
    //~ 3D Vector
    
    // TODO(crazy): Implement
    
    //~ 4D Vector
    
    // TODO(crazy): Implement
    
    
    
    // Helper function implementations
    
    //~ 2D Vector
    
    // TODO(crazy): Implement
    
    //~ 3D Vector
    
    // TODO(crazy): Implement
    
    //~ 4D Vector
    
    // TODO(crazy): Implement
    
}



//~ Acronyms

#ifndef TOOL_NO_ACRONYMS

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

#endif

#endif //_VECTOR_H
