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
    Random rng = { { 1, 2, 3, 4 } };
    TOOL_ASSERT(RandomNext(&rng) == 11520ull);
    TOOL_ASSERT(RandomNext(&rng) == 0ull);
    TOOL_ASSERT(RandomNext(&rng) == 1509978240ull);
    TOOL_ASSERT(RandomNext(&rng) == 1215971899390074240ull);

    // Coordinates pack as 32-bit patterns.
    TOOL_ASSERT(RandomKey(7, -1, 2) == RandomKey(7, 0x00000002ffffffffull));
    TOOL_ASSERT(RandomKey(7, p2 { 3, -4 }) == RandomKey(7, 3, -4));
    TOOL_ASSERT(RandomKey(7, p3 { 3, -4, 5 }) == RandomKey(7, 3, -4, 5, 0));
    TOOL_ASSERT(RandomKey(7, p4 { 3, -4, 5, -6 }) == RandomKey(7, 3, -4, 5, -6));
}



//~ Keyed

static void TestKeyed()
{
    // Distinct keys never collide under one seed, here a 256x256 grid around the origin.
    static u64 values[256 * 256];
    u64 count = 0;
    for (i32 y = -128; y < 128; y++)
        for (i32 x = -128; x < 128; x++)
            values[count++] = RandomKey(12345, x, y);

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

    // Avalanche: flipping any key or seed bit flips each output bit about half the time.
    u32 flips[128][64] = {};
    const u32 samples = 4000;
    Random rng = {};
    RandomInit(&rng, 99);
    for (u32 n = 0; n < samples; n++)
    {
        u64 seed = RandomNext(&rng);
        u64 key = RandomNext(&rng);
        u64 base = RandomKey(seed, key);
        for (u32 bit = 0; bit < 128; bit++)
        {
            u64 flipped = bit < 64 ? RandomKey(seed, key ^ (1ull << bit)) : RandomKey(seed ^ (1ull << (bit - 64)), key);
            u64 diff = base ^ flipped;
            for (u32 out = 0; out < 64; out++)
                flips[bit][out] += (u32)((diff >> out) & 1);
        }
    }
    f64 worst = 0.0;
    for (u32 bit = 0; bit < 128; bit++)
        for (u32 out = 0; out < 64; out++)
        {
            f64 bias = (f64)flips[bit][out] / samples - 0.5;
            worst = bias < 0 ? (-bias > worst ? -bias : worst) : (bias > worst ? bias : worst);
        }
    TOOL_ASSERT(worst < 0.04, "avalanche bias %f", worst); // 5 sigma over 8192 cells is about 0.039

    // Different seeds are unrelated, including adjacent ones.
    u32 equal = 0;
    for (i32 x = 0; x < 1000; x++)
        equal += RandomKey(0, x, 0) == RandomKey(1, x, 0);
    TOOL_ASSERT(equal == 0);
}



//~ Distributions

static void TestRanges()
{
    Random rng = {};
    RandomInit(&rng, 1);

    for (u32 n = 0; n < 100000; n++)
    {
        i8 a = RandomRangeI8(&rng, -128, 127);
        TOOL_ASSERT(a >= -128 && a < 127);
        u8 b = RandomRangeU8(&rng, 250, 255);
        TOOL_ASSERT(b >= 250 && b < 255);
        i16 c = RandomRangeI16(&rng, -5, 5);
        TOOL_ASSERT(c >= -5 && c < 5);
        u16 d = RandomRangeU16(&rng, 0, 1);
        TOOL_ASSERT(d == 0);
        i32 e = RandomRangeI32(&rng, -2147483647 - 1, 2147483647);
        TOOL_ASSERT(e < 2147483647);
        u32 f = RandomRangeU32(&rng, 10, 20);
        TOOL_ASSERT(f >= 10 && f < 20);
        i64 g = RandomRangeI64(&rng, -3, 3);
        TOOL_ASSERT(g >= -3 && g < 3);

        // Just over half the u64 space, so about half the draws hit the rejection path.
        u64 h = RandomRangeU64(&rng, 5, 5 + (1ull << 63) + 1);
        TOOL_ASSERT(h >= 5 && h < 5 + (1ull << 63) + 1);
    }

    // Uniform: chi-square over a range of 10.
    u32 buckets[10] = {};
    const u32 draws = 1000000;
    for (u32 n = 0; n < draws; n++)
        buckets[RandomRangeU32(&rng, 0, 10)]++;
    f64 chi = 0.0;
    for (u32 i = 0; i < 10; i++)
    {
        f64 delta = (f64)buckets[i] - draws / 10.0;
        chi += delta * delta / (draws / 10.0);
    }
    TOOL_ASSERT(chi < 33.7, "chi-square %f", chi); // 9 degrees of freedom, p ~ 0.0001

    // The bits form of a range covers both ends.
    TOOL_ASSERT(RandomRangeI32(0ull, -7, 7) == -7);
    TOOL_ASSERT(RandomRangeI32(~0ull, -7, 7) == 6);
    TOOL_ASSERT(RandomRangeU64(1ull << 20, 3, 9) == 3); // 0 itself falls in the rejection zone
    TOOL_ASSERT(RandomRangeU64(0ull, 3, 9) >= 3 && RandomRangeU64(0ull, 3, 9) < 9);
    TOOL_ASSERT(RandomRangeU64(~0ull, 3, 9) == 8);
}

static void TestFloats()
{
    TOOL_ASSERT(RandomF32(0ull) == 0.0f && RandomF32(~0ull) < 1.0f);
    TOOL_ASSERT(RandomF64(0ull) == 0.0 && RandomF64(~0ull) < 1.0);

    Random rng = {};
    RandomInit(&rng, 2);
    f64 sum = 0.0;
    u32 trues = 0;
    const u32 draws = 1000000;
    for (u32 n = 0; n < draws; n++)
    {
        f32 a = RandomF32(&rng, -2.0f, 3.0f);
        TOOL_ASSERT(a >= -2.0f && a <= 3.0f);
        f64 b = RandomF64(&rng);
        TOOL_ASSERT(b >= 0.0 && b < 1.0);
        sum += b;
        trues += RandomB8(&rng);
    }
    TOOL_ASSERT(sum / draws > 0.498 && sum / draws < 0.502);
    TOOL_ASSERT(trues > draws / 2 - 3000 && trues < draws / 2 + 3000);
}

static void TestGaussianAndGeometry()
{
    Random rng = {};
    RandomInit(&rng, 3);
    const u32 draws = 1000000;

    f64 sum32 = 0.0, square32 = 0.0, sum64 = 0.0, square64 = 0.0;
    for (u32 n = 0; n < draws; n++)
    {
        f64 a = RandomGaussianF32(&rng, 0.0f, 1.0f);
        f64 b = RandomGaussianF64(&rng, 0.0, 1.0);
        sum32 += a;
        square32 += a * a;
        sum64 += b;
        square64 += b * b;
    }
    TOOL_ASSERT(sum32 / draws > -0.005 && sum32 / draws < 0.005);
    TOOL_ASSERT(square32 / draws > 0.99 && square32 / draws < 1.01);
    TOOL_ASSERT(sum64 / draws > -0.005 && sum64 / draws < 0.005);
    TOOL_ASSERT(square64 / draws > 0.99 && square64 / draws < 1.01);
    TOOL_ASSERT(RandomGaussianF32(0ull, 0.0f, 1.0f) == RandomGaussianF32(0ull, 0.0f, 1.0f));

    // On: unit length. In: mean squared radius is 1/2 in a disc and 3/5 in a ball.
    f64 disc = 0.0, ball = 0.0;
    v3 mean = {};
    for (u32 n = 0; n < draws; n++)
    {
        v2 on2 = RandomOnCircle(&rng);
        f32 length2 = on2.x * on2.x + on2.y * on2.y;
        TOOL_ASSERT(length2 > 0.9999f && length2 < 1.0001f);

        v3 on3 = RandomOnSphere(&rng);
        f32 length3 = on3.x * on3.x + on3.y * on3.y + on3.z * on3.z;
        TOOL_ASSERT(length3 > 0.9999f && length3 < 1.0001f);
        mean.x += on3.x;
        mean.y += on3.y;
        mean.z += on3.z;

        v2 in2 = RandomInCircle(&rng);
        f32 r2 = in2.x * in2.x + in2.y * in2.y;
        TOOL_ASSERT(r2 <= 1.0001f);
        disc += r2;

        v3 in3 = RandomInSphere(&rng);
        f32 r3 = in3.x * in3.x + in3.y * in3.y + in3.z * in3.z;
        TOOL_ASSERT(r3 <= 1.0001f);
        ball += r3;
    }
    TOOL_ASSERT(disc / draws > 0.498 && disc / draws < 0.502);
    TOOL_ASSERT(ball / draws > 0.598 && ball / draws < 0.602);
    TOOL_ASSERT(mean.x / draws > -0.005f && mean.x / draws < 0.005f);
    TOOL_ASSERT(mean.z / draws > -0.005f && mean.z / draws < 0.005f);
}

// Counts how many of 'count' points from index 'first' land in each of 'count' equal intervals of one axis.
static b8 RandomSpreadStratified(u64 seed, u64 first, u32 count, u32 axis)
{
    static u32 strata[3125];
    for (u32 i = 0; i < count; i++)
        strata[i] = 0;
    for (u32 i = 0; i < count; i++)
    {
        v3 p = RandomSpreadV3(seed, first + i);
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
        TOOL_ASSERT(RandomSpreadStratified(seed, 0, 1024, 0));
        TOOL_ASSERT(RandomSpreadStratified(seed, 1024 * 37, 1024, 0));
        TOOL_ASSERT(RandomSpreadStratified(seed, 0, 729, 1));
        TOOL_ASSERT(RandomSpreadStratified(seed, 729 * 11, 729, 1));
        TOOL_ASSERT(RandomSpreadStratified(seed, 0, 3125, 2));
        TOOL_ASSERT(RandomSpreadStratified(seed, 3125 * 5, 3125, 2));
    }

    // Lower dimensions are the leading axes of higher ones, and seeds differ.
    u32 differ = 0;
    for (u64 i = 0; i < 1000; i++)
    {
        v3 p = RandomSpreadV3(42, i);
        v2 q = RandomSpreadV2(42, i);
        TOOL_ASSERT(RandomSpreadF32(42, i) == p.x && q.x == p.x && q.y == p.y);
        differ += RandomSpreadV3(43, i).x != p.x;
    }
    TOOL_ASSERT(differ > 990);

    // Reference points, so the integer pipeline can't drift silently.
    v3 a = RandomSpreadV3(0, 0);
    v3 b = RandomSpreadV3(12345, 678);
    TOOL_ASSERT(a.x == 0.784486234f && a.y == 0.600558519f && a.z == 0.326837897f);
    TOOL_ASSERT(b.x == 0.681574941f && b.y == 0.779517949f && b.z == 0.27943933f);
}

static void TestShuffleAndFill()
{
    // Shuffles are permutations, and the same bits give the same order.
    u32 a[100];
    u32 b[100];
    for (u32 i = 0; i < 100; i++)
        a[i] = b[i] = i;
    RandomShuffle(RandomKey(5, 0ull), a, 100);
    RandomShuffle(RandomKey(5, 0ull), b, 100);
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

    Random rng = {};
    RandomInit(&rng, 6);
    RandomShuffle(&rng, a, 100);
    RandomShuffle(&rng, a, 1);
    RandomShuffle(&rng, a, 0);

    // Fill writes each word in little-endian order, the last one partially.
    Random first = {};
    RandomInit(&first, 7);
    Random second = first;
    u8 bytes[19] = {};
    RandomFill(&first, bytes, 19);
    for (u32 word = 0; word < 3; word++)
    {
        u64 bits = RandomNext(&second);
        for (u32 i = 0; i < 8 && word * 8 + i < 19; i++)
            TOOL_ASSERT(bytes[word * 8 + i] == (u8)(bits >> (i * 8)));
    }
    TOOL_ASSERT(RandomNext(&first) == RandomNext(&second));
}



void TestRandom()
{
    TestReference();
    TestKeyed();
    TestRanges();
    TestFloats();
    TestGaussianAndGeometry();
    TestSpread();
    TestShuffleAndFill();
    printf("Random: passed\n");
}
