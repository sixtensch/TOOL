#ifndef _TOOL_MATRIX_H
#define _TOOL_MATRIX_H

#include "basics.h"
#include "vector.h"
#include "mathematics.h"



namespace Tool
{
    
    //- Struct Declarations
    
    //~ 2x2 Matrix
    
    template<typename T>
        struct Mat2
    {
        union
        {
            T m[4];

            // Column-major storage. mIJ = row I, column J.
            struct
            {
                T m00; T m10;
                T m01; T m11;
            };
        };

        Vec2<T> operator*(const Vec2<T>& b); // Transform vector
        
        Mat2<T> operator+(const Mat2<T>& b); // Matrix addition
        Mat2<T> operator-(const Mat2<T>& b); // Matrix subtraction
        Mat2<T> operator*(const Mat2<T>& b); // Matrix multiplication
        Mat2<T> operator*(T b); // Scalar multiplication
        
        Mat2<T>& operator+=(const Mat2<T>& b);
        Mat2<T>& operator-=(const Mat2<T>& b);
        Mat2<T>& operator*=(const Mat2<T>& b);
        Mat2<T>& operator*=(T b);
        
        bool operator==(const Mat2<T>& b); // Equality comparison
        bool operator!=(const Mat2<T>& b); // Inequality comparison
    };
    
    //~ 3x3 Matrix
    
    template<typename T>
        struct Mat3
    {
        union
        {
            T m[9];

            // Column-major storage. mIJ = row I, column J.
            struct
            {
                T m00; T m10; T m20;
                T m01; T m11; T m21;
                T m02; T m12; T m22;
            };
        };
        
        Vec3<T> operator*(const Vec3<T>& b); // Transform vector
        
        Mat3<T> operator+(const Mat3<T>& b); // Matrix addition
        Mat3<T> operator-(const Mat3<T>& b); // Matrix subtraction
        Mat3<T> operator*(const Mat3<T>& b); // Matrix multiplication
        Mat3<T> operator*(T b); // Scalar multiplication
        
        Mat3<T>& operator+=(const Mat3<T>& b);
        Mat3<T>& operator-=(const Mat3<T>& b);
        Mat3<T>& operator*=(const Mat3<T>& b);
        Mat3<T>& operator*=(T b);
        
        bool operator==(const Mat3<T>& b); // Equality comparison
        bool operator!=(const Mat3<T>& b); // Inequality comparison
    };
    
    //~ 4x4 Matrix
    
    template<typename T>
        struct Mat4
    {
        union
        {
            T m[16];

            // Column-major storage. mIJ = row I, column J.
            struct
            {
                T m00; T m10; T m20; T m30;
                T m01; T m11; T m21; T m31;
                T m02; T m12; T m22; T m32;
                T m03; T m13; T m23; T m33;
            };
        };
        
        Vec4<T> operator*(const Vec4<T>& b); // Transform 4D vector
        Vec3<T> operator*(const Vec3<T>& b); // Transform 3D vector, assuming W = 1
        Vec3<T> operator%(const Vec3<T>& b); // Transform 3D vector, assuming W = 0
        
        Mat4<T> operator+(const Mat4<T>& b); // Matrix addition
        Mat4<T> operator-(const Mat4<T>& b); // Matrix subtraction
        Mat4<T> operator*(const Mat4<T>& b); // Matrix multiplication
        Mat4<T> operator*(T b); // Scalar multiplication
        
        Mat4<T>& operator+=(const Mat4<T>& b);
        Mat4<T>& operator-=(const Mat4<T>& b);
        Mat4<T>& operator*=(const Mat4<T>& b);
        Mat4<T>& operator*=(T b);
        
        bool operator==(const Mat4<T>& b); // Equality comparison
        bool operator!=(const Mat4<T>& b); // Inequality comparison
    };
    
    //~ Acronyms
    
    typedef Mat2<f32> m2; // Matrix2x2
    typedef Mat3<f32> m3; // Matrix3x3
    typedef Mat4<f32> m4; // Matrix4x4
    
    typedef Mat2<f64> m2d; // Matrix2x2 double precision
    typedef Mat3<f64> m3d; // Matrix3x3 double precision
    typedef Mat4<f64> m4d; // Matrix4x4 double precision
    
    
    
    //- Helper function declarations
    
    //~ 2x2 Matrix
    
    m2 M2Identity();
    
    m2 M2Transpose(const m2& mat2);
    
    m2 M2Downscale(const m3& mat3);
    m2 M2Downscale(const m4& mat4);
    
    m2 M2Rotation(f32 radians);
    
    m2 M2Scale(v2 scale);
    m2 M2Scale(f32 uniform);
    m2 M2Scale(f32 x, f32 y);
    
    f32 M2Trace(const m2& mat2);
    f32 M2Determinant(const m2& mat2);
    
    //~ 3x3 Matrix
    
    m3 M3Identity();
    
    m3 M3Transpose(const m3& mat3);
    
    m3 M3Upscale(const m2& mat2);
    m3 M3Downscale(const m4& mat4);
    
    m3 M3Rotation(v3 euler);
    m3 M3Rotation(f32 pitch, f32 yaw, f32 roll);
    m3 M3RotationX(f32 pitch);
    m3 M3RotationY(f32 yaw);
    m3 M3RotationZ(f32 roll);
    
    m3 M3Scale(v3 scale);
    m3 M3Scale(f32 uniform);
    m3 M3Scale(f32 x, f32 y, f32 z);
    
    f32 M3Trace(const m4& mat3);
    f32 M3Determinant(const m4& mat3);
    
    //~ 4x4 Matrix
    
    m4 M4Identity();
    
    m4 M4Transpose(const m4& mat4);
    
    m4 M4Upscale(const m2& mat2);
    m4 M4Upscale(const m3& mat3);
    
    m4 M4Rotation(v3 euler);
    m4 M4Rotation(f32 pitch, f32 yaw, f32 roll);
    m4 M4RotationX(f32 pitch);
    m4 M4RotationY(f32 yaw);
    m4 M4RotationZ(f32 roll);
    
    m4 M4Scale(v4 scale);
    m4 M4Scale(v3 scale, f32 w = 1.0f);
    m4 M4Scale(f32 uniform, f32 w = 1.0f);
    m4 M4Scale(f32 x, f32 y, f32 z, f32 w = 1.0f);
    
    m4 M4Translation(v3 position);
    m4 M4Translation(f32 x, f32 y, f32 z);
    
    enum ClipType
    {
        ClipTypeDX,
        ClipTypeVulkan,
        ClipTypeOpenGL
    };
    
    m4 M4ProjectionPerspective(f32 verticalFovDegrees, f32 aspectRatio, f32 near, f32 far, ClipType type);
    m4 M4ProjectionOrthographic(f32 width, f32 height, f32 near, f32 far, ClipType type);
    
    f32 M4Trace(const m4& mat4);
    //f32 M4Determinant(const m4& mat4);
    
    
    
    //- Operator overload implementations
    
    //~ 2x2 Matrix
    
    template<typename T>
        inline Vec2<T> Mat2<T>::operator*(const Vec2<T>& b) 
    {
        return 
        {
            m00 * b.x + m01 * b.y,
            m10 * b.x + m11 * b.y
        };
    }
    
    template<typename T>
        inline Mat2<T> Mat2<T>::operator+(const Mat2<T>& b)
    {
        return
        {
            m00 + b.m00, m10 + b.m10,
            m01 + b.m01, m11 + b.m11
        };
    }
    
    template<typename T>
        inline Mat2<T> Mat2<T>::operator-(const Mat2<T>& b)
    {
        return
        {
            m00 - b.m00, m10 - b.m10,
            m01 - b.m01, m11 - b.m11
        };
    }
    
    template<typename T>
        inline Mat2<T> Mat2<T>::operator*(const Mat2<T>& b)
    {
        return
        {
            m00 * b.m00 + m01 * b.m10,
            m10 * b.m00 + m11 * b.m10,

            m00 * b.m01 + m01 * b.m11,
            m10 * b.m01 + m11 * b.m11
        };
    }
    
    template<typename T>
        inline Mat2<T> Mat2<T>::operator*(T b)
    {
        return
        {
            m00 * b, m10 * b,
            m01 * b, m11 * b
        };
    }
    
    template<typename T>
        inline Mat2<T>& Mat2<T>::operator+=(const Mat2<T>& b) 
    {
        for (int i = 0; i < 4; i++)
        {
            m[i] += b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat2<T>& Mat2<T>::operator-=(const Mat2<T>& b) 
    {
        for (int i = 0; i < 4; i++)
        {
            m[i] -= b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat2<T>& Mat2<T>::operator*=(const Mat2<T>& b) 
    {
        return *this = (*this * b);
    }
    
    template<typename T>
        inline Mat2<T>& Mat2<T>::operator*=(T b) 
    {
        for (int i = 0; i < 4; i++)
        {
            m[i] *= b;
        }
        
        return *this;
    }
    
    template<typename T>
        inline bool Mat2<T>::operator==(const Mat2<T>& b) 
    {
        return
            m00 == b.m00 && m01 == b.m01 &&
            m10 == b.m10 && m11 == b.m11;
    }
    
    template<typename T>
        inline bool Mat2<T>::operator!=(const Mat2<T>& b) 
    {
        return
            m00 != b.m00 || m01 != b.m01 ||
            m10 != b.m10 || m11 != b.m11;
    }
    
    //~ 3x3 Matrix
    
    template<typename T>
        inline Vec3<T> Mat3<T>::operator*(const Vec3<T>& b) 
    {
        return 
        {
            m00 * b.x + m01 * b.y + m02 * b.z,
            m10 * b.x + m11 * b.y + m12 * b.z,
            m20 * b.x + m21 * b.y + m22 * b.z
        };
    }
    
    template<typename T>
        inline Mat3<T> Mat3<T>::operator+(const Mat3<T>& b)
    {
        return
        {
            m00 + b.m00, m10 + b.m10, m20 + b.m20,
            m01 + b.m01, m11 + b.m11, m21 + b.m21,
            m02 + b.m02, m12 + b.m12, m22 + b.m22
        };
    }
    
    template<typename T>
        inline Mat3<T> Mat3<T>::operator-(const Mat3<T>& b)
    {
        return
        {
            m00 - b.m00, m10 - b.m10, m20 - b.m20,
            m01 - b.m01, m11 - b.m11, m21 - b.m21,
            m02 - b.m02, m12 - b.m12, m22 - b.m22
        };
    }
    
    template<typename T>
        inline Mat3<T> Mat3<T>::operator*(const Mat3<T>& b)
    {
        return
        {
            m00 * b.m00 + m01 * b.m10 + m02 * b.m20,
            m10 * b.m00 + m11 * b.m10 + m12 * b.m20,
            m20 * b.m00 + m21 * b.m10 + m22 * b.m20,

            m00 * b.m01 + m01 * b.m11 + m02 * b.m21,
            m10 * b.m01 + m11 * b.m11 + m12 * b.m21,
            m20 * b.m01 + m21 * b.m11 + m22 * b.m21,

            m00 * b.m02 + m01 * b.m12 + m02 * b.m22,
            m10 * b.m02 + m11 * b.m12 + m12 * b.m22,
            m20 * b.m02 + m21 * b.m12 + m22 * b.m22
        };
    }
    
    template<typename T>
        inline Mat3<T> Mat3<T>::operator*(T b)
    {
        return
        {
            m00 * b, m10 * b, m20 * b,
            m01 * b, m11 * b, m21 * b,
            m02 * b, m12 * b, m22 * b
        };
    }
    
    template<typename T>
        inline Mat3<T>& Mat3<T>::operator+=(const Mat3<T>& b) 
    {
        for (int i = 0; i < 9; i++)
        {
            m[i] += b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat3<T>& Mat3<T>::operator-=(const Mat3<T>& b) 
    {
        for (int i = 0; i < 9; i++)
        {
            m[i] -= b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat3<T>& Mat3<T>::operator*=(const Mat3<T>& b) 
    {
        return *this = (*this * b);
    }
    
    template<typename T>
        inline Mat3<T>& Mat3<T>::operator*=(T b) 
    {
        for (int i = 0; i < 9; i++)
        {
            m[i] *= b;
        }
        
        return *this;
    }
    
    template<typename T>
        inline bool Mat3<T>::operator==(const Mat3<T>& b) 
    {
        return
            m00 == b.m00 && m01 == b.m01 && m02 == b.m02 &&
            m10 == b.m10 && m11 == b.m11 && m12 == b.m12 &&
            m20 == b.m20 && m21 == b.m21 && m22 == b.m22;
    }
    
    template<typename T>
        inline bool Mat3<T>::operator!=(const Mat3<T>& b) 
    {
        return
            m00 != b.m00 || m01 != b.m01 || m02 != b.m02 ||
            m10 != b.m10 || m11 != b.m11 || m12 != b.m12 ||
            m20 != b.m20 || m21 != b.m21 || m22 != b.m22;
    }
    
    //~ 4x4 Matrix
    
    template<typename T>
        inline Vec4<T> Mat4<T>::operator*(const Vec4<T>& b) 
    {
        return 
        {
            m00 * b.x + m01 * b.y + m02 * b.z + m03 * b.w,
            m10 * b.x + m11 * b.y + m12 * b.z + m13 * b.w,
            m20 * b.x + m21 * b.y + m22 * b.z + m23 * b.w,
            m30 * b.x + m31 * b.y + m32 * b.z + m33 * b.w
        };
    }
    
    template<typename T>
        inline Vec3<T> Mat4<T>::operator*(const Vec3<T>& b) 
    {
        return 
        {
            m00 * b.x + m01 * b.y + m02 * b.z + m03,
            m10 * b.x + m11 * b.y + m12 * b.z + m13,
            m20 * b.x + m21 * b.y + m22 * b.z + m23,
        };
    }
    
    template<typename T>
        inline Vec3<T> Mat4<T>::operator%(const Vec3<T>& b) 
    {
        return 
        {
            m00 * b.x + m01 * b.y + m02 * b.z,
            m10 * b.x + m11 * b.y + m12 * b.z,
            m20 * b.x + m21 * b.y + m22 * b.z,
        };
    }
    
    template<typename T>
        inline Mat4<T> Mat4<T>::operator+(const Mat4<T>& b)
    {
        return
        {
            m00 + b.m00, m10 + b.m10, m20 + b.m20, m30 + b.m30,
            m01 + b.m01, m11 + b.m11, m21 + b.m21, m31 + b.m31,
            m02 + b.m02, m12 + b.m12, m22 + b.m22, m32 + b.m32,
            m03 + b.m03, m13 + b.m13, m23 + b.m23, m33 + b.m33
        };
    }
    
    template<typename T>
        inline Mat4<T> Mat4<T>::operator-(const Mat4<T>& b)
    {
        return
        {
            m00 - b.m00, m10 - b.m10, m20 - b.m20, m30 - b.m30,
            m01 - b.m01, m11 - b.m11, m21 - b.m21, m31 - b.m31,
            m02 - b.m02, m12 - b.m12, m22 - b.m22, m32 - b.m32,
            m03 - b.m03, m13 - b.m13, m23 - b.m23, m33 - b.m33
        };
    }
    
    template<typename T>
        inline Mat4<T> Mat4<T>::operator*(const Mat4<T>& b)
    {
        return
        {
            m00 * b.m00 + m01 * b.m10 + m02 * b.m20 + m03 * b.m30,
            m10 * b.m00 + m11 * b.m10 + m12 * b.m20 + m13 * b.m30,
            m20 * b.m00 + m21 * b.m10 + m22 * b.m20 + m23 * b.m30,
            m30 * b.m00 + m31 * b.m10 + m32 * b.m20 + m33 * b.m30,

            m00 * b.m01 + m01 * b.m11 + m02 * b.m21 + m03 * b.m31,
            m10 * b.m01 + m11 * b.m11 + m12 * b.m21 + m13 * b.m31,
            m20 * b.m01 + m21 * b.m11 + m22 * b.m21 + m23 * b.m31,
            m30 * b.m01 + m31 * b.m11 + m32 * b.m21 + m33 * b.m31,

            m00 * b.m02 + m01 * b.m12 + m02 * b.m22 + m03 * b.m32,
            m10 * b.m02 + m11 * b.m12 + m12 * b.m22 + m13 * b.m32,
            m20 * b.m02 + m21 * b.m12 + m22 * b.m22 + m23 * b.m32,
            m30 * b.m02 + m31 * b.m12 + m32 * b.m22 + m33 * b.m32,

            m00 * b.m03 + m01 * b.m13 + m02 * b.m23 + m03 * b.m33,
            m10 * b.m03 + m11 * b.m13 + m12 * b.m23 + m13 * b.m33,
            m20 * b.m03 + m21 * b.m13 + m22 * b.m23 + m23 * b.m33,
            m30 * b.m03 + m31 * b.m13 + m32 * b.m23 + m33 * b.m33
        };
    }
    
    template<typename T>
        inline Mat4<T> Mat4<T>::operator*(T b)
    {
        return
        {
            m00 * b, m10 * b, m20 * b, m30 * b,
            m01 * b, m11 * b, m21 * b, m31 * b,
            m02 * b, m12 * b, m22 * b, m32 * b,
            m03 * b, m13 * b, m23 * b, m33 * b
        };
    }
    
    template<typename T>
        inline Mat4<T>& Mat4<T>::operator+=(const Mat4<T>& b) 
    {
        for (int i = 0; i < 16; i++)
        {
            m[i] += b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat4<T>& Mat4<T>::operator-=(const Mat4<T>& b) 
    {
        for (int i = 0; i < 16; i++)
        {
            m[i] -= b.m[i];
        }
        
        return *this;
    }
    
    template<typename T>
        inline Mat4<T>& Mat4<T>::operator*=(const Mat4<T>& b) 
    {
        return *this = (*this * b);
    }
    
    template<typename T>
        inline Mat4<T>& Mat4<T>::operator*=(T b) 
    {
        for (int i = 0; i < 16; i++)
        {
            m[i] *= b;
        }
        
        return *this;
    }
    
    template<typename T>
        inline bool Mat4<T>::operator==(const Mat4<T>& b) 
    {
        return
            m00 == b.m00 && m01 == b.m01 && m02 == b.m02 && m03 == b.m03 &&
            m10 == b.m10 && m11 == b.m11 && m12 == b.m12 && m13 == b.m13 &&
            m20 == b.m20 && m21 == b.m21 && m22 == b.m22 && m23 == b.m23 &&
            m30 == b.m30 && m31 == b.m31 && m32 == b.m32 && m33 == b.m33;
    }
    
    template<typename T>
        inline bool Mat4<T>::operator!=(const Mat4<T>& b) 
    {
        return
            m00 != b.m00 || m01 != b.m01 || m02 != b.m02 || m03 != b.m03 ||
            m10 != b.m10 || m11 != b.m11 || m12 != b.m12 || m13 != b.m13 ||
            m20 != b.m20 || m21 != b.m21 || m22 != b.m22 || m23 != b.m23 ||
            m30 != b.m30 || m31 != b.m31 || m32 != b.m32 || m33 != b.m33;
    }
    
    
    
    //- Helper function definitions
    
    //~ 2x2 Matrix
    
    inline m2 M2Identity()
    {
        return 
        { 
            1.0f, 0.0f,
            0.0f, 1.0f 
        };
    }
    
    inline m2 M2Transpose(const m2& mat2)
    {
        // r.mIJ = mat2.mJI
        return
        {
            mat2.m00, mat2.m01,
            mat2.m10, mat2.m11,
        };
    }
    
    inline m2 M2Downscale(const m3& mat3)
    {
        return
        {
            mat3.m00, mat3.m10,
            mat3.m01, mat3.m11
        };
    }

    inline m2 M2Downscale(const m4& mat4)
    {
        return
        {
            mat4.m00, mat4.m10,
            mat4.m01, mat4.m11
        };
    }
    
    inline m2 M2Rotation(f32 radians)
    {
        f32 c = FCos(radians);
        f32 s = FSin(radians);

        // [ c -s ; s c ]
        return
        {
            c, s,
            -s, c
        };
    }
    
    inline m2 M2Scale(v2 scale)
    {
        return 
        {
            scale.x, 0,
            0, scale.y
        };
    }
    
    inline m2 M2Scale(f32 uniform)
    {
        return 
        {
            uniform, 0,
            0, uniform
        };
    }
    
    inline m2 M2Scale(f32 x, f32 y)
    {
        return
        {
            x, 0,
            0, y
        };
    }
    
    inline f32 M2Trace(const m2& mat2)
    {
        return mat2.m00 + mat2.m11;
    }
    
    inline f32 M2Determinant(const m2& mat2)
    {
        return mat2.m00 * mat2.m11 - mat2.m01 * mat2.m10;
    }
    
    //~ 3x3 Matrix
    
    inline m3 M3Identity()
    {
        return 
        {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        };
    }
    
    inline m3 M3Transpose(const m3& mat3)
    {
        // r.mIJ = mat3.mJI
        return
        {
            mat3.m00, mat3.m01, mat3.m02,
            mat3.m10, mat3.m11, mat3.m12,
            mat3.m20, mat3.m21, mat3.m22
        };
    }

    inline m3 M3Upscale(const m2& mat2)
    {
        return
        {
            mat2.m00, mat2.m10, 0,
            mat2.m01, mat2.m11, 0,
            0, 0, 1
        };
    }

    inline m3 M3Downscale(const m4& mat4)
    {
        return
        {
            mat4.m00, mat4.m10, mat4.m20,
            mat4.m01, mat4.m11, mat4.m21,
            mat4.m02, mat4.m12, mat4.m22
        };
    }
    
    inline m3 M3Rotation(v3 euler)
    {
        return M3Rotation(euler.x, euler.y, euler.z);
    }
    
    inline m3 M3Rotation(f32 pitch, f32 yaw, f32 roll)
    {
        f32 cx = FCos(pitch);
        f32 sx = FSin(pitch);
        f32 cy = FCos(yaw);
        f32 sy = FSin(yaw);
        f32 cz = FCos(roll);
        f32 sz = FSin(roll);
        
        f32 sxsy = sx * sy;
        f32 sxcy = sx * cy;

        return
        {
            cy * cz + sxsy * sz,   cx * sz,               sxcy * sz - sy * cz,
            sxsy * cz - cy * sz,   cx * cz,               sxcy * cz + sy * sz,
            cx * sy,               -sx,                   cx * cy
        };
    }

    inline m3 M3RotationX(f32 pitch)
    {
        f32 c = FCos(pitch);
        f32 s = FSin(pitch);

        // [ 1 0 0 ; 0 c -s ; 0 s c ]
        return
        {
            1, 0, 0,
            0, c, s,
            0, -s, c
        };
    }

    inline m3 M3RotationY(f32 yaw)
    {
        f32 c = FCos(yaw);
        f32 s = FSin(yaw);

        // [ c 0 s ; 0 1 0 ; -s 0 c ]
        return
        {
            c, 0, -s,
            0, 1, 0,
            s, 0, c
        };
    }

    inline m3 M3RotationZ(f32 roll)
    {
        f32 c = FCos(roll);
        f32 s = FSin(roll);

        // [ c -s 0 ; s c 0 ; 0 0 1 ]
        return
        {
            c, s, 0,
            -s, c, 0,
            0, 0, 1
        };
    }
    
    inline m3 M3Scale(v3 scale)
    {
        return 
        {
            scale.x, 0, 0,
            0, scale.y, 0,
            0, 0, scale.z
        };
    }
    
    inline m3 M3Scale(f32 uniform)
    {
        return 
        {
            uniform, 0, 0,
            0, uniform, 0,
            0, 0, uniform
        };
    }
    
    inline m3 M3Scale(f32 x, f32 y, f32 z)
    {
        return
        {
            x, 0, 0,
            0, y, 0,
            0, 0, z
        };
    }
    
    inline f32 M3Trace(const m4& mat3)
    {
        return mat3.m00 + mat3.m11 + mat3.m22;
    }
    
    inline f32 M3Determinant(const m4& mat3) 
    {
        return
            mat3.m00 * mat3.m11 * mat3.m22 +
            mat3.m10 * mat3.m21 * mat3.m02 +
            mat3.m20 * mat3.m01 * mat3.m12 -
            mat3.m00 * mat3.m21 * mat3.m12 -
            mat3.m10 * mat3.m01 * mat3.m22 -
            mat3.m20 * mat3.m11 * mat3.m02;
    }
    
    //~ 4x4 Matrix
    
    inline m4 M4Identity()
    {
        return
        {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }
    
    inline m4 M4Transpose(const m4& mat4)
    {
        // r.mIJ = mat4.mJI
        return
        {
            mat4.m00, mat4.m01, mat4.m02, mat4.m03,
            mat4.m10, mat4.m11, mat4.m12, mat4.m13,
            mat4.m20, mat4.m21, mat4.m22, mat4.m23,
            mat4.m30, mat4.m31, mat4.m32, mat4.m33
        };
    }

    inline m4 M4Upscale(const m2& mat2)
    {
        return
        {
            mat2.m00, mat2.m10, 0, 0,
            mat2.m01, mat2.m11, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }

    inline m4 M4Upscale(const m3& mat3)
    {
        return
        {
            mat3.m00, mat3.m10, mat3.m20, 0,
            mat3.m01, mat3.m11, mat3.m21, 0,
            mat3.m02, mat3.m12, mat3.m22, 0,
            0, 0, 0, 1
        };
    }
    
    inline m4 M4Rotation(v3 euler)
    {
        return M4Rotation(euler.x, euler.y, euler.z);
    }
    
    inline m4 M4Rotation(f32 pitch, f32 yaw, f32 roll)
    {
        f32 cx = FCos(pitch);
        f32 sx = FSin(pitch);
        f32 cy = FCos(yaw);
        f32 sy = FSin(yaw);
        f32 cz = FCos(roll);
        f32 sz = FSin(roll);
        
        f32 sxsy = sx * sy;
        f32 sxcy = sx * cy;

        return
        {
            cy * cz + sxsy * sz,   cx * sz,               sxcy * sz - sy * cz,   0,
            sxsy * cz - cy * sz,   cx * cz,               sxcy * cz + sy * sz,   0,
            cx * sy,               -sx,                   cx * cy,               0,
            0,                     0,                     0,                     1
        };
    }

    inline m4 M4RotationX(f32 pitch)
    {
        f32 c = FCos(pitch);
        f32 s = FSin(pitch);

        // [ 1 0 0 0 ; 0 c -s 0 ; 0 s c 0 ; 0 0 0 1 ]
        return
        {
            1, 0, 0, 0,
            0, c, s, 0,
            0, -s, c, 0,
            0, 0, 0, 1
        };
    }

    inline m4 M4RotationY(f32 yaw)
    {
        f32 c = FCos(yaw);
        f32 s = FSin(yaw);

        // [ c 0 s 0 ; 0 1 0 0 ; -s 0 c 0 ; 0 0 0 1 ]
        return
        {
            c, 0, -s, 0,
            0, 1, 0, 0,
            s, 0, c, 0,
            0, 0, 0, 1
        };
    }

    inline m4 M4RotationZ(f32 roll)
    {
        f32 c = FCos(roll);
        f32 s = FSin(roll);

        // [ c -s 0 0 ; s c 0 0 ; 0 0 1 0 ; 0 0 0 1 ]
        return
        {
            c, s, 0, 0,
            -s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
    }
    
    inline m4 M4Scale(v4 scale)
    {
        return
        {
            scale.x, 0, 0, 0,
            0, scale.y, 0, 0,
            0, 0, scale.z, 0,
            0, 0, 0, scale.w
        };
    }
    
    inline m4 M4Scale(v3 scale, f32 w)
    {
        return
        {
            scale.x, 0, 0, 0,
            0, scale.y, 0, 0,
            0, 0, scale.z, 0,
            0, 0, 0, w
        };
    }
    
    inline m4 M4Scale(f32 uniform, f32 w)
    {
        return
        {
            uniform, 0, 0, 0,
            0, uniform, 0, 0,
            0, 0, uniform, 0,
            0, 0, 0, w 
        };
    }
    
    inline m4 M4Scale(f32 x, f32 y, f32 z, f32 w)
    {
        return
        {
            x, 0, 0, 0,
            0, y, 0, 0,
            0, 0, z, 0,
            0, 0, 0, w
        };
    }
    
    inline m4 M4Translation(v3 position)
    {
        return
        {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            position.x, position.y, position.z, 1
        };
    }

    inline m4 M4Translation(f32 x, f32 y, f32 z)
    {
        return
        {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            x, y, z, 1
        };
    }
    
    inline m4 M4ProjectionPerspective(f32 verticalFovDegrees, f32 aspectRatio, f32 nearClip, f32 farClip, ClipType type)
    {
        f32 yFactor = 1.0f - (type == ClipTypeVulkan) * 2.0f;
        f32 zFactor = 1.0f - (type == ClipTypeOpenGL) * 2.0f;
        
        f32 r = TOOL_D2R(verticalFovDegrees);
        
        f32 yy = 1.0f / FTan(r * 0.5f);
        f32 xx = yy / aspectRatio; 
        
        f32 zz = farClip / (farClip - nearClip);
        f32 zw = -nearClip * zz;

        // rows: [xx 0 0 0] [0 yy*yFac 0 0] [0 0 zz*zFac zw] [0 0 1 0]
        return
        {
            xx, 0, 0, 0,
            0, yy * yFactor, 0, 0,
            0, 0, zz * zFactor, 1,
            0, 0, zw, 0
        };
    }
    
    inline m4 M4ProjectionOrthographic(f32 width, f32 height, f32 nearClip, f32 farClip, ClipType type)
    {
        return {};
    }
    
    inline f32 M4Trace(const m4& mat4)
    {
        return mat4.m00 + mat4.m11 + mat4.m22 + mat4.m33;
    }
    
    /*inline f32 M4Determinant(const m4& mat4)
    {
        
    }*/
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::m2; // Matrix2x2
using Tool::m3; // Matrix3x3
using Tool::m4; // Matrix4x4

using Tool::m2d; // Matrix2x2 double precision
using Tool::m3d; // Matrix3x3 double precision
using Tool::m4d; // Matrix4x4 double precision

#endif



#endif //_MATRIX_H
