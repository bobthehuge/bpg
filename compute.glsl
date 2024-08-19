#version 430

// #define BLOB_COUNT 256
#define BLOB_COUNT 4

#define EPS 0.000001f
#define INT_MAX 0x7FFFFFFF
#define UINT_MAX 0xFFFFFFFFu
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308
#define PI 3.1415f
#define TWOPI 6.28318530717958647692f
#define OCTAVES 8
#define STRENGH 1

layout (local_size_x = 4, local_size_y = 1, local_size_z = 1) in;
precision highp float;

layout(std430, binding = 1) buffer blobsPosSrcLayout {
    ivec2 src[BLOB_COUNT];
};

// layout(std430, binding = 2) writeonly buffer blobsPosDstLayout {
//     uvec2 dst[BLOB_COUNT];
// };

layout(std430, binding = 2) buffer blobsRotLayout {
    int rots[BLOB_COUNT];
};

uniform uint iFrame;
uniform vec2 iResolution;
uniform float iTime;

uint hash(uint state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

// float randf(uint m)
// {
//     const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
//     const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

//     m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
//     m |= ieeeOne; // Add fractional part to 1.0

//     float  f = uintBitsToFloat( m ); // Range [1:2]
//     return f - 1.0; // Range [0:1]
// }

// uint rand(uvec3 p)
// {
//     return hash(
//         p.y * uint(iResolution.x) + p.x + 
//         hash(int(p.z + iTime * 100000))
//     );
// }

float randf(uvec3 p)
{
    return float(hash(
        p.y * uint(iResolution.x) + p.x + 
        hash(int(float(p.z) + iTime * 100000.0))
    )) / float(UINT_MAX);
}

float randf(uint id) 
{
    float r = randf(uvec3(src[id].xy, uint(id)));
    r = isnan(r) ? 0.0 : isinf(r) ? 1.0 : r;
    return r;
}

// float fmodf(float a, float b)
// {
//     return mod((mod(a, b) + b), b);
// }

void main()
{
    bool even_frame = iFrame % 2u == 0;
    ivec2 iRes = ivec2(iResolution);

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        float r = intBitsToFloat(rots[i]);

        if (iFrame == 0)
        {
            r = randf(i);
            atomicExchange(rots[i], floatBitsToInt(r));
        }

        float a = r * TWOPI;
        
        int x = int(src[i].x);
        int y = int(src[i].y);

        x = int(float(x) + cos(a));
        y = int(float(y) + sin(a));

        if (x <= 0 || y <= 0 || x >= iRes.x - 1 || y >= iRes.y - 1)
        {
            r = randf(i);

            x = min(iRes.x - 1, max(0, x));
            y = min(iRes.y - 1, max(0, y));

            atomicExchange(rots[i], floatBitsToInt(r));
        }

        // if (x <= 0)
        // {
        //     r = randf(i);
        //     x = 0;
        // }
        // if (x >= iRes.x - 1)
        // {
        //     r = randf(i);
        //     x = iRes.x - 1;
        // }
        // if (y <= 0)
        // {
        //     r = randf(i);
        //     y = 0;
        // }
        // if (y >= iRes.y - 1)
        // {
        //     r = randf(i);
        //     y = iRes.y - 1;
        // }

        atomicExchange(src[i].x, x);
        atomicExchange(src[i].y, y);
    }
}
