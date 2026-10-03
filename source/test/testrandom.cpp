#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



//~ Reference values

// nasam, as published.
static_assert(RandomMix(0) == 0);
static_assert(RandomMix(1) == 0x9c1a051e07b9e10dull);
static_assert(RandomMix(0x0123456789abcdefull) == 0x770f13a0ab5b163dull);

static void TestReference()
{
    // xoshiro256** from state {1, 2, 3, 4}, as published.
    RandomStream stream = { { 1, 2, 3, 4 } };
    TOOL_ASSERT(RandomU64Next(&stream) == 11520ull);
    TOOL_ASSERT(RandomU64Next(&stream) == 0ull);
    TOOL_ASSERT(RandomU64Next(&stream) == 1509978240ull);
    TOOL_ASSERT(RandomU64Next(&stream) == 1215971899390074240ull);

    // Key shapes: coordinates fill 32-bit slots, missing ones are 0, and IDs fill the first two.
    RandomKeyed keyed = {};
    RandomKeyedInit(&keyed, 7);
    TOOL_ASSERT(RandomU64At(&keyed, {-1, 2}) == RandomU64At(&keyed, 0x00000002ffffffffull));
    TOOL_ASSERT(RandomU64At(&keyed, {3, -4}) == RandomU64At(&keyed, {3, -4, 0}));
    TOOL_ASSERT(RandomU64At(&keyed, {3, -4}) == RandomU64At(&keyed, {3, -4, 0, 0}));
    TOOL_ASSERT(RandomU64At(&keyed, -5) == RandomU64At(&keyed, {-5, 0}));
    TOOL_ASSERT(RandomU64At(&keyed, 5u) == RandomU64At(&keyed, {5, 0}));
    TOOL_ASSERT(RandomU64At(&keyed, (i64)-5) == RandomU64At(&keyed, {-5, -1}));
    TOOL_ASSERT(RandomU64At(&keyed, p2 { 3, -4 }) == RandomU64At(&keyed, {3, -4}));
    TOOL_ASSERT(RandomU64At(&keyed, {p2 { 3, -4 }, 9}) == RandomU64At(&keyed, {3, -4, 9}));
    TOOL_ASSERT(RandomU64At(&keyed, p3 { 3, -4, 5 }) == RandomU64At(&keyed, {3, -4, 5}));
    TOOL_ASSERT(RandomU64At(&keyed, {p3 { 3, -4, 5 }, 9}) == RandomU64At(&keyed, {3, -4, 5, 9}));
    TOOL_ASSERT(RandomU64At(&keyed, p4 { 3, -4, 5, -6 }) == RandomU64At(&keyed, {3, -4, 5, -6}));
    TOOL_ASSERT(RandomU64At(&keyed, {0x00000002ffffffffull, 9}) == RandomU64At(&keyed, {-1, 2, 9}));

    // A zeroed keyed generator is seed 0.
    RandomKeyed zero = {};
    RandomKeyed seeded = {};
    RandomKeyedInit(&seeded, 0);
    TOOL_ASSERT(RandomU64At(&zero, {1, 2}) == RandomU64At(&seeded, {1, 2}));
}



//~ Keyed

static void TestKeyed()
{
    RandomKeyed keyed = {};
    RandomKeyedInit(&keyed, 12345);

    // Distinct keys never collide under one seed, here a 256x256 grid around the origin.
    static u64 values[256 * 256];
    u64 count = 0;
    for (i32 y = -128; y < 128; y++)
        for (i32 x = -128; x < 128; x++)
            values[count++] = RandomU64At(&keyed, {x, y});

    for (u64 i = 0; i < count; i++)
        for (u64 j = i + 1; j < i + 300 && j < count; j++)
            TOOL_ASSERT(values[i] != values[j]);

    // Low bits of neighboring cells are spread evenly: chi-square over 256 buckets of the bottom byte.
    u32 buckets[256] = {};
    for (u64 i = 0; i < count; i++)
        buckets[values[i] & 0xff]++;
    f64 chi = 0.0;
    for (u32 i = 0; i < 256; i++)
    {
        f64 delta = (f64)buckets[i] - (f64)count / 256.0;
        chi += delta * delta / ((f64)count / 256.0);
    }
    TOOL_ASSERT(chi < 350.0, "chi-square %f", chi); // 255 degrees of freedom, p ~ 0.0001

    // Avalanche: flipping any key or seed bit flips each output bit about half the time. Bits 0-63 are the first
    // key word, 64-127 the second (nonzero, so the third round runs), 128-191 the seed.
    static u32 flips[192][64] = {};
    const u32 samples = 4000;
    RandomStream stream = {};
    RandomStreamInit(&stream, 99);
    for (u32 n = 0; n < samples; n++)
    {
        RandomKeyed base = { RandomU64Next(&stream) };
        RandomKey key = {};
        key.low = RandomU64Next(&stream);
        key.high = RandomU64Next(&stream) | 1;
        u64 reference = RandomU64At(&base, key);
        for (u32 bit = 0; bit < 192; bit++)
        {
            RandomKeyed flippedSeed = base;
            RandomKey flippedKey = key;
            if (bit < 64)
                flippedKey.low ^= 1ull << bit;
            else if (bit < 128)
                flippedKey.high ^= 1ull << (bit - 64);
            else
                flippedSeed.seed ^= 1ull << (bit - 128);
            if (flippedKey.high == 0)
                continue;
            u64 diff = reference ^ RandomU64At(&flippedSeed, flippedKey);
            for (u32 out = 0; out < 64; out++)
                flips[bit][out] += (u32)((diff >> out) & 1);
        }
    }
    f64 worst = 0.0;
    for (u32 bit = 0; bit < 192; bit++)
        for (u32 out = 0; out < 64; out++)
        {
            f64 bias = (f64)flips[bit][out] / samples - 0.5;
            worst = bias < 0 ? (-bias > worst ? -bias : worst) : (bias > worst ? bias : worst);
        }
    TOOL_ASSERT(worst < 0.04, "avalanche bias %f", worst); // 5 sigma over 12288 cells is about 0.04

    // Different seeds are unrelated, including adjacent ones.
    RandomKeyed other = {};
    RandomKeyedInit(&other, 12346);
    u32 equal = 0;
    for (i32 x = 0; x < 1000; x++)
        equal += RandomU64At(&keyed, {x, 0}) == RandomU64At(&other, {x, 0});
    TOOL_ASSERT(equal == 0);
}



//~ Getters

static void TestIntegers()
{
    RandomStream stream = {};
    RandomStreamInit(&stream, 1);

    for (u32 n = 0; n < 100000; n++)
    {
        i8 a = RandomI8Next(&stream, -128, 127);
        TOOL_ASSERT(a >= -128 && a < 127);
        u8 b = RandomU8Next(&stream, 250, 255);
        TOOL_ASSERT(b >= 250 && b < 255);
        i16 c = RandomI16Next(&stream, -5, 5);
        TOOL_ASSERT(c >= -5 && c < 5);
        u16 d = RandomU16Next(&stream, 0, 1);
        TOOL_ASSERT(d == 0);
        i32 e = RandomI32Next(&stream, -2147483647 - 1, 2147483647);
        TOOL_ASSERT(e < 2147483647);
        u32 f = RandomU32Next(&stream, 10, 20);
        TOOL_ASSERT(f >= 10 && f < 20);
        i64 g = RandomI64Next(&stream, -3, 3);
        TOOL_ASSERT(g >= -3 && g < 3);

        // Just over half the u64 space, so about half the draws hit the rejection path.
        u64 h = RandomU64Next(&stream, 5, 5 + (1ull << 63) + 1);
        TOOL_ASSERT(h >= 5 && h < 5 + (1ull << 63) + 1);
    }

    // Uniform: chi-square over a range of 10.
    u32 buckets[10] = {};
    const u32 draws = 1000000;
    for (u32 n = 0; n < draws; n++)
        buckets[RandomU32Next(&stream, 0, 10)]++;
    f64 chi = 0.0;
    for (u32 i = 0; i < 10; i++)
    {
        f64 delta = (f64)buckets[i] - draws / 10.0;
        chi += delta * delta / (draws / 10.0);
    }
    TOOL_ASSERT(chi < 33.7, "chi-square %f", chi); // 9 degrees of freedom, p ~ 0.0001

    // Ranges reach both ends.
    TOOL_ASSERT(RandomBitsI32(0ull, -7, 7) == -7);
    TOOL_ASSERT(RandomBitsI32(~0ull, -7, 7) == 6);
    TOOL_ASSERT(RandomBitsU64(1ull << 20, 3, 9) == 3); // 0 itself falls in the rejection zone
    TOOL_ASSERT(RandomBitsU64(~0ull, 3, 9) == 8);

    // Keyed getters are the same conversions over the keyed word.
    RandomKeyed keyed = {};
    RandomKeyedInit(&keyed, 4);
    for (i32 x = 0; x < 1000; x++)
    {
        u64 bits = RandomU64At(&keyed, {x, 3});
        TOOL_ASSERT(RandomI32At(&keyed, {x, 3}, -10, 10) == RandomBitsI32(bits, -10, 10));
        TOOL_ASSERT(RandomU8At(&keyed, {x, 3}) == (u8)(bits >> 56));
        TOOL_ASSERT(RandomI32At(&keyed, {x, 3}) == RandomI32At(&keyed, {x, 3}));
    }
}

static void TestFloats()
{
    TOOL_ASSERT(RandomBitsF32(0ull) == 0.0f && RandomBitsF32(~0ull) < 1.0f);
    TOOL_ASSERT(RandomBitsF64(0ull) == 0.0 && RandomBitsF64(~0ull) < 1.0);

    RandomStream stream = {};
    RandomStreamInit(&stream, 2);
    f64 sum = 0.0;
    u32 trues = 0;
    const u32 draws = 1000000;
    for (u32 n = 0; n < draws; n++)
    {
        f32 a = RandomF32Next(&stream, -2.0f, 3.0f);
        TOOL_ASSERT(a >= -2.0f && a <= 3.0f);
        f64 b = RandomF64Next(&stream);
        TOOL_ASSERT(b >= 0.0 && b < 1.0);
        sum += b;
        trues += RandomB8Next(&stream);
    }
    TOOL_ASSERT(sum / draws > 0.498 && sum / draws < 0.502);
    TOOL_ASSERT(trues > draws / 2 - 3000 && trues < draws / 2 + 3000);

    // Index 0 at a key is the plain key; other indices are independent values.
    RandomKeyed keyed = {};
    RandomKeyedInit(&keyed, 5);
    TOOL_ASSERT(RandomF32At(&keyed, {8, 9, 0}) == RandomF32At(&keyed, {8, 9}));
    TOOL_ASSERT(RandomF32At(&keyed, {8, 9, 1}) != RandomF32At(&keyed, {8, 9}));
}

static void TestPoints()
{
    RandomStream stream = {};
    RandomStreamInit(&stream, 3);
    const u32 draws = 1000000;

    f64 sum32 = 0.0, square32 = 0.0, sum64 = 0.0, square64 = 0.0;
    for (u32 n = 0; n < draws; n++)
    {
        f64 a = RandomGaussianF32Next(&stream, 0.0f, 1.0f);
        f64 b = RandomGaussianF64Next(&stream, 0.0, 1.0);
        sum32 += a;
        square32 += a * a;
        sum64 += b;
        square64 += b * b;
    }
    TOOL_ASSERT(sum32 / draws > -0.005 && sum32 / draws < 0.005);
    TOOL_ASSERT(square32 / draws > 0.99 && square32 / draws < 1.01);
    TOOL_ASSERT(sum64 / draws > -0.005 && sum64 / draws < 0.005);
    TOOL_ASSERT(square64 / draws > 0.99 && square64 / draws < 1.01);

    // On: unit length. In: mean squared radius is 1/2 in a disc and 3/5 in a ball. Unit square and cube: mean 1/2.
    f64 disc = 0.0, ball = 0.0;
    v3 mean = {};
    v3 cube = {};
    for (u32 n = 0; n < draws; n++)
    {
        v2 on2 = RandomOnCircleNext(&stream);
        f32 length2 = on2.x * on2.x + on2.y * on2.y;
        TOOL_ASSERT(length2 > 0.9999f && length2 < 1.0001f);

        v3 on3 = RandomOnSphereNext(&stream);
        f32 length3 = on3.x * on3.x + on3.y * on3.y + on3.z * on3.z;
        TOOL_ASSERT(length3 > 0.9999f && length3 < 1.0001f);
        mean.x += on3.x;
        mean.y += on3.y;
        mean.z += on3.z;

        v2 in2 = RandomInCircleNext(&stream);
        f32 r2 = in2.x * in2.x + in2.y * in2.y;
        TOOL_ASSERT(r2 <= 1.0001f);
        disc += r2;

        v3 in3 = RandomInSphereNext(&stream);
        f32 r3 = in3.x * in3.x + in3.y * in3.y + in3.z * in3.z;
        TOOL_ASSERT(r3 <= 1.0001f);
        ball += r3;

        v3 c = RandomV3Next(&stream);
        TOOL_ASSERT(c.x >= 0.0f && c.x < 1.0f && c.y >= 0.0f && c.y < 1.0f && c.z >= 0.0f && c.z < 1.0f);
        cube.x += c.x;
        cube.y += c.y;
        cube.z += c.z;
    }
    TOOL_ASSERT(disc / draws > 0.498 && disc / draws < 0.502);
    TOOL_ASSERT(ball / draws > 0.598 && ball / draws < 0.602);
    TOOL_ASSERT(mean.x / draws > -0.005f && mean.x / draws < 0.005f);
    TOOL_ASSERT(mean.z / draws > -0.005f && mean.z / draws < 0.005f);
    TOOL_ASSERT(cube.x / draws > 0.498f && cube.x / draws < 0.502f);
    TOOL_ASSERT(cube.y / draws > 0.498f && cube.y / draws < 0.502f);
    TOOL_ASSERT(cube.z / draws > 0.498f && cube.z / draws < 0.502f);
}



//~ Spread

// Whether 'count' points from index 'first' land one in each of 'count' equal intervals of one axis.
static b8 TestSpreadStratified(const RandomSpread* spread, u64 first, u32 count, u32 axis)
{
    static u32 strata[3125];
    for (u32 i = 0; i < count; i++)
        strata[i] = 0;
    for (u32 i = 0; i < count; i++)
    {
        v3 p = RandomV3At(spread, first + i);
        f32 value = axis == 0 ? p.x : axis == 1 ? p.y : p.z;
        TOOL_ASSERT(value >= 0.0f && value < 1.0f);
        strata[(u32)((f64)value * count)]++;
    }
    for (u32 i = 0; i < count; i++)
        if (strata[i] != 1)
            return false;
    return true;
}

static void TestSpread()
{
    // Each aligned run of b^k indices has one point per 1/b^k interval on the base-b axis, for any seed.
    for (u64 seed = 0; seed < 20; seed++)
    {
        RandomSpread spread = {};
        RandomSpreadInit(&spread, seed);
        TOOL_ASSERT(TestSpreadStratified(&spread, 0, 1024, 0));
        TOOL_ASSERT(TestSpreadStratified(&spread, 1024 * 37, 1024, 0));
        TOOL_ASSERT(TestSpreadStratified(&spread, 0, 729, 1));
        TOOL_ASSERT(TestSpreadStratified(&spread, 729 * 11, 729, 1));
        TOOL_ASSERT(TestSpreadStratified(&spread, 0, 3125, 2));
        TOOL_ASSERT(TestSpreadStratified(&spread, 3125 * 5, 3125, 2));
    }

    // A zeroed spread gives plain Halton points.
    RandomSpread plain = {};
    TOOL_ASSERT(RandomV3At(&plain, 1).x == 0.5f && RandomV3At(&plain, 1).y == RandomBitsF32(0x5555560000000000ull));
    TOOL_ASSERT(TestSpreadStratified(&plain, 0, 729, 1));

    // Lower dimensions are the leading axes of higher ones, and seeds differ.
    RandomSpread spread = {};
    RandomSpreadInit(&spread, 42);
    RandomSpread other = {};
    RandomSpreadInit(&other, 43);
    u32 differ = 0;
    for (u64 i = 0; i < 1000; i++)
    {
        v3 p = RandomV3At(&spread, i);
        v2 q = RandomV2At(&spread, i);
        TOOL_ASSERT(RandomF32At(&spread, i) == p.x && q.x == p.x && q.y == p.y);
        differ += RandomV3At(&other, i).x != p.x;
    }
    TOOL_ASSERT(differ > 990);

    // Reference points, so the integer pipeline can't drift silently.
    RandomSpread zeroSeed = {};
    RandomSpreadInit(&zeroSeed, 0);
    RandomSpread otherSeed = {};
    RandomSpreadInit(&otherSeed, 12345);
    v3 a = RandomV3At(&zeroSeed, 0);
    v3 b = RandomV3At(&otherSeed, 678);
    TOOL_ASSERT(a.x == 0.784486234f && a.y == 0.600558579f && a.z == 0.326837957f);
    TOOL_ASSERT(b.x == 0.681574941f && b.y == 0.779518008f && b.z == 0.27943939f);
}



//~ Collections

static void TestCollections()
{
    // Shuffles are permutations, and the same key gives the same order.
    RandomKeyed keyed = {};
    RandomKeyedInit(&keyed, 5);
    u32 a[100];
    u32 b[100];
    for (u32 i = 0; i < 100; i++)
        a[i] = b[i] = i;
    RandomShuffleAt(&keyed, 0ull, a, 100);
    RandomShuffleAt(&keyed, 0ull, b, 100);
    u32 seen[100] = {};
    u32 moved = 0;
    for (u32 i = 0; i < 100; i++)
    {
        TOOL_ASSERT(a[i] == b[i]);
        seen[a[i]]++;
        moved += a[i] != i;
    }
    for (u32 i = 0; i < 100; i++)
        TOOL_ASSERT(seen[i] == 1);
    TOOL_ASSERT(moved > 80);

    RandomStream stream = {};
    RandomStreamInit(&stream, 6);
    RandomShuffleNext(&stream, a, 100);
    RandomShuffleNext(&stream, a, 1);
    RandomShuffleNext(&stream, a, 0);

    // Fill writes each word in little-endian order, the last one partially.
    RandomStream first = {};
    RandomStreamInit(&first, 7);
    RandomStream second = first;
    u8 bytes[19] = {};
    RandomFillNext(&first, bytes, 19);
    for (u32 word = 0; word < 3; word++)
    {
        u64 bits = RandomU64Next(&second);
        for (u32 i = 0; i < 8 && word * 8 + i < 19; i++)
            TOOL_ASSERT(bytes[word * 8 + i] == (u8)(bits >> (i * 8)));
    }
    TOOL_ASSERT(RandomU64Next(&first) == RandomU64Next(&second));
}



void TestRandom()
{
    TestReference();
    TestKeyed();
    TestIntegers();
    TestFloats();
    TestPoints();
    TestSpread();
    TestCollections();
    printf("Random: passed\n");
}
