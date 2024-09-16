#version 430

// #define BLOB_COUNT 1000000
#define BLOB_COUNT 16384
// #define BLOB_COUNT 4096
// #define BLOB_COUNT 2048
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4
#define UINT_MAX 0xFFFFFFFFu
#define PI 3.1415f
#define TWOPI 6.2831f
#define SENSOR_SIZE 10
#define TURN_SPEED 0.5 * TWOPI

#define LOCAL_SIZE 256

layout (local_size_x = LOCAL_SIZE, local_size_y = 1) in;

layout(std430, binding = 1) buffer blobsPosLayout {
    vec2 src[BLOB_COUNT];
};

layout(std430, binding = 2) buffer blobsRotLayout {
    float rots[BLOB_COUNT];
};

// layout(binding = 3, r32ui) uniform uimage2D iTex;
layout(binding = 3, rgba8) uniform image2D iTex;
// uniform writeonly image2D iTex;

uniform int iFrame;
uniform ivec2 iResolution;
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

float sense(uint id, float offset)
{
    float a = rots[id] + offset;
    vec2 dir = vec2(cos(a), sin(a));
    ivec2 pos = ivec2(src[id] + dir * offset);

    float sum = 0;

    for (int offx = -SENSOR_SIZE; offx < SENSOR_SIZE; offx++)
    {
        for (int offy = -SENSOR_SIZE; offy < SENSOR_SIZE; offy++)
        {
            ivec2 npos = pos + ivec2(offx, offy);

            if (npos.x >= 0 && npos.x < iResolution.x &&
                npos.y >= 0 && npos.y < iResolution.y
            ) {
                sum += imageLoad(iTex, npos).r;
            }
        }
    }

    return sum;
}

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (i >= BLOB_COUNT)
        return;
    
    vec2 fRes = vec2(iResolution);
    
    float r = rots[i];
    float a = r * TWOPI;
    float rand = randf(i);

    // forward
    float wf = sense(i, 0);
    // left
    float wl = sense(i, a);
    // right
    float wr = sense(i, -a);

    if (wf > wl && wf > wr)
        a += 0;
    else if (wf < wl && wf < wr)
        a += (rand - 0.5) * 2 * TURN_SPEED;
    else if (wr > wl)
        a -= rand * TURN_SPEED;
    else if (wl > wr)
        a += rand * TURN_SPEED;
    
    vec2 vel = vec2(cos(a), sin(a));
    vec2 pos = src[i] + vel;

    if (pos.x <= 0.0 || pos.y <= 0.0 || 
        pos.x >= fRes.x - 1.0 || pos.y >= fRes.y - 1.0 ||
        pos.x == src[i].x || pos.y == src[i].y) 
    {
        pos.x = min(fRes.x - 1, max(0, pos.x));
        pos.y = min(fRes.y - 1, max(0, pos.y));

        rots[i] = rand;
    }

    src[i] = pos;

    imageStore(iTex, ivec2(pos), vec4(1.0, 1.0, 1.0, 1.0));
}
