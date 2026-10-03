#include "tool/intrinsics.h"

// Freestanding: no C runtime under Clang, so the same file runs as wasm without one.
#if defined(_MSC_VER) && !defined(__clang__)
#include <cmath>
#endif

using namespace Tool;



//~ Bookkeeping

static u32 testFailures;
static const c8* testFirstFailure;

static void Check(b8 ok, const c8* name)
{
    if (ok)
        return;
    if (testFailures == 0)
        testFirstFailure = name;
    testFailures++;
}

//~ Scalar references

#if defined(_MSC_VER) && !defined(__clang__)
static f32 RefSqrt(f32 x) { return std::sqrt(x); }
static f64 RefSqrt(f64 x) { return std::sqrt(x); }
static f32 RefFloor(f32 x) { return std::floor(x); }
static f64 RefFloor(f64 x) { return std::floor(x); }
static f32 RefCeil(f32 x) { return std::ceil(x); }
static f64 RefCeil(f64 x) { return std::ceil(x); }
static f32 RefRound(f32 x) { return std::nearbyint(x); }
static f64 RefRound(f64 x) { return std::nearbyint(x); }
static f32 RefFma(f32 a, f32 b, f32 c) { return std::fma(a, b, c); }
static f64 RefFma(f64 a, f64 b, f64 c) { return std::fma(a, b, c); }
#else
static f32 RefSqrt(f32 x) { return __builtin_sqrtf(x); }
static f64 RefSqrt(f64 x) { return __builtin_sqrt(x); }
static f32 RefFloor(f32 x) { return __builtin_floorf(x); }
static f64 RefFloor(f64 x) { return __builtin_floor(x); }
static f32 RefCeil(f32 x) { return __builtin_ceilf(x); }
static f64 RefCeil(f64 x) { return __builtin_ceil(x); }
// nearbyint rounds ties to even in the default mode, and unlike roundeven every C runtime has it.
static f32 RefRound(f32 x) { return __builtin_nearbyintf(x); }
static f64 RefRound(f64 x) { return __builtin_nearbyint(x); }
#if defined(__wasm__)
// wasm has no FMA, so MulAdd never fuses there, and without a C runtime there is no fma() to call.
static f32 RefFma(f32 a, f32 b, f32 c) { return a * b + c; }
static f64 RefFma(f64 a, f64 b, f64 c) { return a * b + c; }
#else
static f32 RefFma(f32 a, f32 b, f32 c) { return __builtin_fmaf(a, b, c); }
static f64 RefFma(f64 a, f64 b, f64 c) { return __builtin_fma(a, b, c); }
#endif
#endif

static u32 Bits(f32 x) { union { f32 f; u32 u; } b = { x }; return b.u; }
static u64 Bits(f64 x) { union { f64 f; u64 u; } b = { x }; return b.u; }
static f32 F32Bits(u32 x) { union { u32 u; f32 f; } b = { x }; return b.f; }
static f64 F64Bits(u64 x) { union { u64 u; f64 f; } b = { x }; return b.f; }

// Bit-exact, except that any NaN matches any NaN: payload and sign of a produced NaN differ between targets.
template<typename F>
static b8 Same(F a, F b)
{
    if (a != a || b != b)
        return a != a && b != b;
    return Bits(a) == Bits(b);
}

template<typename F>
static F Abs(F x) { return x < 0 ? -x : x; }

//~ Inputs

static const f32 f32Values[] = {
    0.0f, -0.0f, 0.5f, -0.5f, 1.5f, -1.5f, 2.5f, -2.5f, 0.49999997f, -0.49999997f, 1.0f, -1.0f, 3.75f, -3.25f,
    8388607.5f, -8388607.5f, 8388608.0f, -8388609.0f, 16777215.0f, 3.0e9f, -3.0e9f, 1.0e-40f, -1.0e-40f,
    123.456f, -7.9f, 0.1f, 1.0e30f, -1.0e30f, 6.0f, 1.0e-3f, -0.7f, 0.3f,
};

static const f64 f64Values[] = {
    0.0, -0.0, 0.5, -0.5, 1.5, -1.5, 2.5, -2.5, 0.49999999999999994, -0.49999999999999994, 1.0, -1.0, 3.75, -3.25,
    4503599627370495.5, -4503599627370495.5, 4503599627370496.0, -4503599627370497.0, 9007199254740991.0, 3.0e18,
    -3.0e18, 1.0e-310, -1.0e-310, 123.456, -7.9, 0.1, 1.0e300, -1.0e300, 6.0, 1.0e-3, -0.7, 0.3, 2147483648.5,
};

static const f32 f32Positive[] = { 0.5f, 1.5f, 2.5f, 3.75f, 123.456f, 0.1f, 6.0f, 1.0e-3f, 1.0e30f, 8388608.0f, 1.0f, 7.0e-20f };

static const u32 f32Count = sizeof(f32Values) / sizeof(f32Values[0]);
static const u32 f64Count = sizeof(f64Values) / sizeof(f64Values[0]);
static const u32 f32PositiveCount = sizeof(f32Positive) / sizeof(f32Positive[0]);

static const u32 intCount = 64;
static i8 i8Values[intCount];
static i16 i16Values[intCount];
static i32 i32Values[intCount];
static i64 i64Values[intCount];

static void FillIntegers()
{
    u64 random = 0x123456789abcdefull;
    for (u32 i = 0; i < intCount; i++)
    {
        random = random * 6364136223846793005ull + 1442695040888963407ull;
        u64 bits = random ^ (random >> 29);
        i8Values[i] = (i8)bits;
        i16Values[i] = (i16)bits;
        i32Values[i] = (i32)bits;
        i64Values[i] = (i64)bits;
    }

    i8 i8Edges[] = { 0, 1, -1, 127, -128, 2, -2, 100, -100 };
    i16 i16Edges[] = { 0, 1, -1, 32767, -32768, 2, -2, 300, -300 };
    i32 i32Edges[] = { 0, 1, -1, 2147483647, (i32)0x80000000, 2, -2, 70000, -70000 };
    i64 i64Edges[] = { 0, 1, -1, 0x7fffffffffffffffll, (i64)0x8000000000000000ull, 2, -2, 0x100000000ll, -0x100000001ll };
    for (u32 i = 0; i < 9; i++)
    {
        i8Values[i] = i8Edges[i];
        i16Values[i] = i16Edges[i];
        i32Values[i] = i32Edges[i];
        i64Values[i] = i64Edges[i];
    }
}

// Lane i of input set k, for step s: different sets see different mixes of the values.
static u32 Pick(u32 s, u32 i, u32 k, u32 count) { return (s + i * (2 * k + 1) + k * 5) % count; }

//~ Lane traits

#define TOOL_TEST_TRAITS(Name, Vector, Lane, Count, Load, Store)                                                       \
    struct Name                                                                                                        \
    {                                                                                                                  \
        typedef Vector V;                                                                                              \
        typedef Lane T;                                                                                                \
        static const u32 N = Count;                                                                                    \
        static V LoadV(const T* p) { return Load(p); }                                                                 \
        static void StoreV(V v, T* p) { Store(v, p); }                                                                 \
    };

TOOL_TEST_TRAITS(F32x4T, f32_x4, f32, 4, F32x4Load, F32x4Store)
TOOL_TEST_TRAITS(F32x8T, f32_x8, f32, 8, F32x8Load, F32x8Store)
TOOL_TEST_TRAITS(F64x2T, f64_x2, f64, 2, F64x2Load, F64x2Store)
TOOL_TEST_TRAITS(F64x4T, f64_x4, f64, 4, F64x4Load, F64x4Store)
TOOL_TEST_TRAITS(I8x16T, i8_x16, i8, 16, I8x16Load, I8x16Store)
TOOL_TEST_TRAITS(I8x32T, i8_x32, i8, 32, I8x32Load, I8x32Store)
TOOL_TEST_TRAITS(I16x8T, i16_x8, i16, 8, I16x8Load, I16x8Store)
TOOL_TEST_TRAITS(I16x16T, i16_x16, i16, 16, I16x16Load, I16x16Store)
TOOL_TEST_TRAITS(I32x4T, i32_x4, i32, 4, I32x4Load, I32x4Store)
TOOL_TEST_TRAITS(I32x8T, i32_x8, i32, 8, I32x8Load, I32x8Store)
TOOL_TEST_TRAITS(I64x2T, i64_x2, i64, 2, I64x2Load, I64x2Store)
TOOL_TEST_TRAITS(I64x4T, i64_x4, i64, 4, I64x4Load, I64x4Store)

template<typename T> static const T* ValuesOf();
template<> const f32* ValuesOf<f32>() { return f32Values; }
template<> const f64* ValuesOf<f64>() { return f64Values; }
template<> const i8* ValuesOf<i8>() { return i8Values; }
template<> const i16* ValuesOf<i16>() { return i16Values; }
template<> const i32* ValuesOf<i32>() { return i32Values; }
template<> const i64* ValuesOf<i64>() { return i64Values; }

template<typename T> static u32 CountOf() { return intCount; }
template<> u32 CountOf<f32>() { return f32Count; }
template<> u32 CountOf<f64>() { return f64Count; }

//~ Drivers
// Run 'op' over many lane mixes of the inputs, checking every output lane against 'check(out, inputs...)'.

template<typename L, typename Op, typename Ref>
static void Unary(const c8* name, Op op, Ref ref)
{
    typedef typename L::T T;
    const T* values = ValuesOf<T>();
    u32 count = CountOf<T>();
    for (u32 s = 0; s < count; s++)
    {
        T a[L::N], out[L::N];
        for (u32 i = 0; i < L::N; i++)
            a[i] = values[Pick(s, i, 0, count)];
        L::StoreV(op(L::LoadV(a)), out);
        for (u32 i = 0; i < L::N; i++)
            Check(ref(out[i], a[i]), name);
    }
}

template<typename L, typename Op, typename Ref>
static void Binary(const c8* name, Op op, Ref ref)
{
    typedef typename L::T T;
    const T* values = ValuesOf<T>();
    u32 count = CountOf<T>();
    for (u32 s = 0; s < count * 3; s++)
    {
        T a[L::N], b[L::N], out[L::N];
        for (u32 i = 0; i < L::N; i++)
        {
            a[i] = values[Pick(s, i, 0, count)];
            b[i] = values[Pick(s / 3, i, 1 + s % 3, count)];
        }
        L::StoreV(op(L::LoadV(a), L::LoadV(b)), out);
        for (u32 i = 0; i < L::N; i++)
            Check(ref(out[i], a[i], b[i], i), name);
    }
}

template<typename L, typename Op, typename Ref>
static void Ternary(const c8* name, Op op, Ref ref)
{
    typedef typename L::T T;
    const T* values = ValuesOf<T>();
    u32 count = CountOf<T>();
    for (u32 s = 0; s < count * 3; s++)
    {
        T a[L::N], b[L::N], c[L::N], out[L::N];
        for (u32 i = 0; i < L::N; i++)
        {
            a[i] = values[Pick(s, i, 0, count)];
            b[i] = values[Pick(s / 3, i, 1 + s % 3, count)];
            c[i] = values[Pick(s, i, 4, count)];
        }
        L::StoreV(op(L::LoadV(a), L::LoadV(b), L::LoadV(c)), out);
        for (u32 i = 0; i < L::N; i++)
            Check(ref(out[i], a[i], b[i], c[i]), name);
    }
}

//~ Float tests

template<typename F> static F BitAnd(F a, F b);
template<> f32 BitAnd(f32 a, f32 b) { return F32Bits(Bits(a) & Bits(b)); }
template<> f64 BitAnd(f64 a, f64 b) { return F64Bits(Bits(a) & Bits(b)); }
template<typename F> static F BitAndNot(F a, F b);
template<> f32 BitAndNot(f32 a, f32 b) { return F32Bits(Bits(a) & ~Bits(b)); }
template<> f64 BitAndNot(f64 a, f64 b) { return F64Bits(Bits(a) & ~Bits(b)); }
template<typename F> static F BitOr(F a, F b);
template<> f32 BitOr(f32 a, f32 b) { return F32Bits(Bits(a) | Bits(b)); }
template<> f64 BitOr(f64 a, f64 b) { return F64Bits(Bits(a) | Bits(b)); }
template<typename F> static F BitXor(F a, F b);
template<> f32 BitXor(f32 a, f32 b) { return F32Bits(Bits(a) ^ Bits(b)); }
template<> f64 BitXor(f64 a, f64 b) { return F64Bits(Bits(a) ^ Bits(b)); }

// Bitwise results are compared exactly: NaN inputs pass through bit operations unchanged.
template<typename F> static b8 SameBits(F a, F b) { return Bits(a) == Bits(b); }

template<typename F> static b8 MaskIs(F lane, b8 set)
{
    return set ? Bits(lane) == (decltype(Bits(lane)))~(decltype(Bits(lane)))0 : Bits(lane) == 0;
}

template<Comparison C, typename F> static b8 RefCompare(F a, F b)
{
    if constexpr (C == ComparisonEquals) return a == b;
    else if constexpr (C == ComparisonNotEquals) return a < b || a > b;
    else if constexpr (C == ComparisonGreaterThan) return a > b;
    else if constexpr (C == ComparisonGreaterThanOrEquals) return a >= b;
    else if constexpr (C == ComparisonLessThan) return a < b;
    else return a <= b;
}

#define TOOL_TEST_COMPARE(L, Function, C)                                                                              \
    Binary<L>(#Function "<" #C ">", [](L::V a, L::V b) { return Function<C>(a, b); },                                  \
              [](L::T out, L::T a, L::T b, u32) { return MaskIs(out, RefCompare<C>(a, b)); })

#define TOOL_TEST_FLOAT(L, P)                                                                                          \
    Binary<L>(#P "Add", P##Add, [](L::T out, L::T a, L::T b, u32) { return Same(out, (L::T)(a + b)); });               \
    Binary<L>(#P "Sub", P##Sub, [](L::T out, L::T a, L::T b, u32) { return Same(out, (L::T)(a - b)); });               \
    Binary<L>(#P "Mul", P##Mul, [](L::T out, L::T a, L::T b, u32) { return Same(out, (L::T)(a * b)); });               \
    Binary<L>(#P "Div", P##Div, [](L::T out, L::T a, L::T b, u32) { return Same(out, (L::T)(a / b)); });               \
    Unary<L>(#P "Sqrt", P##Sqrt, [](L::T out, L::T a) { return Same(out, RefSqrt(a)); });                              \
    Unary<L>(#P "Ceil", P##Ceil, [](L::T out, L::T a) { return Same(out, RefCeil(a)); });                              \
    Unary<L>(#P "Floor", P##Floor, [](L::T out, L::T a) { return Same(out, RefFloor(a)); });                           \
    Unary<L>(#P "Round", P##Round, [](L::T out, L::T a) { return Same(out, RefRound(a)); });                           \
    Ternary<L>(#P "MulAdd", P##MulAdd, [](L::T out, L::T a, L::T b, L::T c) {                                          \
        return Same(out, RefFma(a, b, c)) || Same(out, (L::T)((L::T)(a * b) + c));                                     \
    });                                                                                                                \
    Ternary<L>(#P "MulSub", P##MulSub, [](L::T out, L::T a, L::T b, L::T c) {                                          \
        return Same(out, RefFma(a, b, -c)) || Same(out, (L::T)((L::T)(a * b) - c));                                    \
    });                                                                                                                \
    Binary<L>(#P "AddSub", P##AddSub, [](L::T out, L::T a, L::T b, u32 i) {                                            \
        return Same(out, (L::T)(i % 2 == 0 ? a - b : a + b));                                                          \
    });                                                                                                                \
    Binary<L>(#P "And", P##And, [](L::T out, L::T a, L::T b, u32) { return SameBits(out, BitAnd(a, b)); });            \
    Binary<L>(#P "AndNot", P##AndNot, [](L::T out, L::T a, L::T b, u32) { return SameBits(out, BitAndNot(a, b)); });   \
    Binary<L>(#P "Or", P##Or, [](L::T out, L::T a, L::T b, u32) { return SameBits(out, BitOr(a, b)); });               \
    Binary<L>(#P "Xor", P##Xor, [](L::T out, L::T a, L::T b, u32) { return SameBits(out, BitXor(a, b)); });            \
    Binary<L>(#P "Blend", [](L::V a, L::V b) { return P##Blend(a, b, P##Compare<ComparisonLessThan>(a, b)); },         \
              [](L::T out, L::T a, L::T b, u32) { return SameBits(out, a < b ? b : a); });                             \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonEquals);                                                                \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonNotEquals);                                                             \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonGreaterThan);                                                           \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonGreaterThanOrEquals);                                                   \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonLessThan);                                                              \
    TOOL_TEST_COMPARE(L, P##Compare, ComparisonLessThanOrEquals);

// Inv and InvSqrt may be the x86 estimate: relative error under 1.5 * 2^-12.
template<typename L, typename Op, typename Ref>
static void Approximate(const c8* name, Op op, Ref ref)
{
    for (u32 s = 0; s < f32PositiveCount; s++)
    {
        f32 a[L::N], out[L::N];
        for (u32 i = 0; i < L::N; i++)
            a[i] = f32Positive[(s + i) % f32PositiveCount];
        L::StoreV(op(L::LoadV(a)), out);
        for (u32 i = 0; i < L::N; i++)
        {
            f32 expected = ref(a[i]);
            Check(Abs(out[i] - expected) <= expected * (1.5f / 4096.0f), name);
        }
    }
}

static void TestFloats()
{
    TOOL_TEST_FLOAT(F32x4T, F32x4)
    TOOL_TEST_FLOAT(F32x8T, F32x8)
    TOOL_TEST_FLOAT(F64x2T, F64x2)
    TOOL_TEST_FLOAT(F64x4T, F64x4)

    Approximate<F32x4T>("F32x4Inv", F32x4Inv, [](f32 a) { return 1.0f / a; });
    Approximate<F32x8T>("F32x8Inv", F32x8Inv, [](f32 a) { return 1.0f / a; });
    Approximate<F32x4T>("F32x4InvSqrt", F32x4InvSqrt, [](f32 a) { return (f32)(1.0 / RefSqrt((f64)a)); });
    Approximate<F32x8T>("F32x8InvSqrt", F32x8InvSqrt, [](f32 a) { return (f32)(1.0 / RefSqrt((f64)a)); });
}

//~ Integer tests
// References compute in unsigned arithmetic, which wraps like the lanes do.

template<typename T> struct UnsignedOf;
template<> struct UnsignedOf<i8> { typedef u8 U; };
template<> struct UnsignedOf<i16> { typedef u16 U; };
template<> struct UnsignedOf<i32> { typedef u32 U; };
template<> struct UnsignedOf<i64> { typedef u64 U; };

template<typename T> static T WrapAdd(T a, T b) { typedef typename UnsignedOf<T>::U U; return (T)(U)((U)a + (U)b); }
template<typename T> static T WrapSub(T a, T b) { typedef typename UnsignedOf<T>::U U; return (T)(U)((U)a - (U)b); }
template<typename T> static T WrapMul(T a, T b) { typedef typename UnsignedOf<T>::U U; return (T)(U)((u64)(U)a * (u64)(U)b); }
template<typename T> static T WrapAbs(T a) { typedef typename UnsignedOf<T>::U U; return a < 0 ? (T)(U)((U)0 - (U)a) : a; }

#define TOOL_TEST_INT(L, P)                                                                                            \
    Binary<L>(#P "Add", P##Add, [](L::T out, L::T a, L::T b, u32) { return out == WrapAdd(a, b); });                   \
    Binary<L>(#P "Sub", P##Sub, [](L::T out, L::T a, L::T b, u32) { return out == WrapSub(a, b); });                   \
    Unary<L>(#P "Abs", P##Abs, [](L::T out, L::T a) { return out == WrapAbs(a); });

#define TOOL_TEST_INT_MUL(L, P)                                                                                        \
    Binary<L>(#P "Mul", P##Mul, [](L::T out, L::T a, L::T b, u32) { return out == WrapMul(a, b); });

static void TestIntegers()
{
    TOOL_TEST_INT(I8x16T, I8x16)
    TOOL_TEST_INT(I8x32T, I8x32)
    TOOL_TEST_INT(I16x8T, I16x8)
    TOOL_TEST_INT(I16x16T, I16x16)
    TOOL_TEST_INT(I32x4T, I32x4)
    TOOL_TEST_INT(I32x8T, I32x8)
    TOOL_TEST_INT(I64x2T, I64x2)
    TOOL_TEST_INT(I64x4T, I64x4)

    TOOL_TEST_INT_MUL(I16x8T, I16x8)
    TOOL_TEST_INT_MUL(I16x16T, I16x16)
    TOOL_TEST_INT_MUL(I32x4T, I32x4)
    TOOL_TEST_INT_MUL(I32x8T, I32x8)
    TOOL_TEST_INT_MUL(I64x2T, I64x2)
    TOOL_TEST_INT_MUL(I64x4T, I64x4)
}

//~ Set, load and lane order
// Lane 0 is the first argument and the lowest address everywhere.

template<typename L>
static void ExpectLanes(const c8* name, typename L::V v, const typename L::T* expected)
{
    typename L::T out[L::N];
    L::StoreV(v, out);
    for (u32 i = 0; i < L::N; i++)
        Check(out[i] == expected[i], name);
}

static void TestSet()
{
    f32 f[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    f64 d[4] = { 1, 2, 3, 4 };
    i8 b[32];
    i16 h[16];
    i32 w[8] = { 1, -2, 3, -4, 5, -6, 7, -8 };
    i64 q[4] = { 1, -2, 0x100000000ll, -0x300000000ll };
    for (u32 i = 0; i < 32; i++)
        b[i] = (i8)(i * 7 - 50);
    for (u32 i = 0; i < 16; i++)
        h[i] = (i16)(i * 1000 - 7000);

    ExpectLanes<F32x4T>("F32x4Set", F32x4Set(1, 2, 3, 4), f);
    ExpectLanes<F32x8T>("F32x8Set", F32x8Set(1, 2, 3, 4, 5, 6, 7, 8), f);
    ExpectLanes<F64x2T>("F64x2Set", F64x2Set(1, 2), d);
    ExpectLanes<F64x4T>("F64x4Set", F64x4Set(1, 2, 3, 4), d);
    ExpectLanes<I64x2T>("I64x2Set", I64x2Set(q[0], q[1]), q);
    ExpectLanes<I64x4T>("I64x4Set", I64x4Set(q[0], q[1], q[2], q[3]), q);
    ExpectLanes<I32x4T>("I32x4Set", I32x4Set(w[0], w[1], w[2], w[3]), w);
    ExpectLanes<I32x8T>("I32x8Set", I32x8Set(w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]), w);
    ExpectLanes<I16x8T>("I16x8Set", I16x8Set(h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]), h);
    ExpectLanes<I16x16T>("I16x16Set", I16x16Set(h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], h[8], h[9], h[10], h[11], h[12], h[13], h[14], h[15]), h);
    ExpectLanes<I8x16T>("I8x16Set", I8x16Set(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]), b);
    ExpectLanes<I8x32T>("I8x32Set", I8x32Set(b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15],
                                              b[16], b[17], b[18], b[19], b[20], b[21], b[22], b[23], b[24], b[25], b[26], b[27], b[28], b[29], b[30], b[31]), b);

    v4 vec4[2] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 } };
    v2 vec2[4] = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } };
    v2d vec2d[2] = { { 1, 2 }, { 3, 4 } };
    v4d vec4d = { 1, 2, 3, 4 };
    p4 point4[2] = { { 1, -2, 3, -4 }, { 5, -6, 7, -8 } };
    p2 point2[4] = { { 1, -2 }, { 3, -4 }, { 5, -6 }, { 7, -8 } };

    ExpectLanes<F32x4T>("F32x4Set(v4)", F32x4Set(vec4[0]), f);
    ExpectLanes<F32x4T>("F32x4Set(v2, v2)", F32x4Set(vec2[0], vec2[1]), f);
    ExpectLanes<F32x8T>("F32x8Set(v4, v4)", F32x8Set(vec4[0], vec4[1]), f);
    ExpectLanes<F32x8T>("F32x8Set(v2 x4)", F32x8Set(vec2[0], vec2[1], vec2[2], vec2[3]), f);
    ExpectLanes<F64x2T>("F64x2Set(v2d)", F64x2Set(vec2d[0]), d);
    ExpectLanes<F64x4T>("F64x4Set(v4d)", F64x4Set(vec4d), d);
    ExpectLanes<F64x4T>("F64x4Set(v2d, v2d)", F64x4Set(vec2d[0], vec2d[1]), d);
    ExpectLanes<I32x4T>("I32x4Set(p4)", I32x4Set(point4[0]), w);
    ExpectLanes<I32x4T>("I32x4Set(p2, p2)", I32x4Set(point2[0], point2[1]), w);
    ExpectLanes<I32x8T>("I32x8Set(p4, p4)", I32x8Set(point4[0], point4[1]), w);
    ExpectLanes<I32x8T>("I32x8Set(p2 x4)", I32x8Set(point2[0], point2[1], point2[2], point2[3]), w);

    ExpectLanes<F32x4T>("F32x4Load(v4)", F32x4Load(&vec4[0]), f);
    ExpectLanes<F32x4T>("F32x4Load(v2)", F32x4Load(vec2), f);
    ExpectLanes<F32x8T>("F32x8Load(v4)", F32x8Load(vec4), f);
    ExpectLanes<F32x8T>("F32x8Load(v2)", F32x8Load(vec2), f);
    ExpectLanes<F64x2T>("F64x2Load(v2d)", F64x2Load(vec2d), d);
    ExpectLanes<F64x4T>("F64x4Load(v2d)", F64x4Load(vec2d), d);
    ExpectLanes<F64x4T>("F64x4Load(v4d)", F64x4Load(&vec4d), d);

    // Broadcasts
    f32 f5[8] = { 5, 5, 5, 5, 5, 5, 5, 5 };
    f64 d5[4] = { 5, 5, 5, 5 };
    i8 b5[32];
    i16 h5[16];
    i32 w5[8];
    i64 q5[4];
    for (u32 i = 0; i < 32; i++) b5[i] = -5;
    for (u32 i = 0; i < 16; i++) h5[i] = -500;
    for (u32 i = 0; i < 8; i++) w5[i] = -50000;
    for (u32 i = 0; i < 4; i++) q5[i] = -5000000000ll;

    ExpectLanes<F32x4T>("F32x4Set(all)", F32x4Set(5.0f), f5);
    ExpectLanes<F32x8T>("F32x8Set(all)", F32x8Set(5.0f), f5);
    ExpectLanes<F64x2T>("F64x2Set(all)", F64x2Set(5.0), d5);
    ExpectLanes<F64x4T>("F64x4Set(all)", F64x4Set(5.0), d5);
    ExpectLanes<F32x4T>("F32x4LoadOne", F32x4LoadOne(&f5[0]), f5);
    ExpectLanes<F64x2T>("F64x2LoadOne", F64x2LoadOne(&d5[0]), d5);
    ExpectLanes<I8x16T>("I8x16Set(all)", I8x16Set(-5), b5);
    ExpectLanes<I8x32T>("I8x32Set(all)", I8x32Set(-5), b5);
    ExpectLanes<I16x8T>("I16x8Set(all)", I16x8Set(-500), h5);
    ExpectLanes<I16x16T>("I16x16Set(all)", I16x16Set(-500), h5);
    ExpectLanes<I32x4T>("I32x4Set(all)", I32x4Set(-50000), w5);
    ExpectLanes<I32x8T>("I32x8Set(all)", I32x8Set(-50000), w5);
    ExpectLanes<I64x2T>("I64x2Set(all)", I64x2Set(-5000000000ll), q5);
    ExpectLanes<I64x4T>("I64x4Set(all)", I64x4Set(-5000000000ll), q5);

    // Unaligned loads and stores
    f32 unaligned[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    ExpectLanes<F32x8T>("F32x8Load unaligned", F32x8Load(unaligned + 1), f);
}

//~ Entry

u32 TestIntrinsicsRun(const c8** outFirstFailure)
{
    testFailures = 0;
    testFirstFailure = nullptr;

    FillIntegers();
    TestSet();
    TestFloats();
    TestIntegers();

    if (outFirstFailure != nullptr)
        *outFirstFailure = testFirstFailure;
    return testFailures;
}
