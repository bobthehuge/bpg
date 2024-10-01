#version 430

#define BLOB_COUNT 1000000
// #define BLOB_COUNT 100000
// #define BLOB_COUNT 4096
// #define BLOB_COUNT 2048
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4
#define UINT_MAX 0xF5F5F5FFu
#define PI 3.1415f
#define TWOPI 6.2831f
// #define SENSOR_SIZE 3
// #define TURN_SPEED 0.5 * TWOPI

layout (local_size_x = 32) in;

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
uniform float iRotSpeed;
uniform int iSensorSize;
uniform float iSenseAngle;
uniform float iDstOffset;

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
        uint(p.y) * iResolution.x + uint(p.x) + 
        hash(int(float(p.z) + iTime * 100000.0))
    )) / float(UINT_MAX);
}

float randf(uint id) 
{
    float r = randf(uvec3(src[id].xy, id.x));
    r = isnan(r) ? 0.0 : isinf(r) ? 1.0 : r;
    return r;
}

float sense(uint id, float offset)
{
    float a = rots[id] + offset;
    vec2 dir = vec2(cos(a), sin(a));
    ivec2 pos = ivec2(src[id] + dir * iDstOffset);

    float sum = 0;

    for (int offx = -iSensorSize; offx < iSensorSize; offx++)
    {
        for (int offy = -iSensorSize; offy < iSensorSize; offy++)
        {
            int posx = min(iResolution.x - 1, max(0, pos.x + offx));
            int posy = min(iResolution.y - 1, max(0, pos.y + offy));

            sum += imageLoad(iTex, ivec2(posx, posy)).r;
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
    
    float a = rots[i];
    float rand = randf(i);
    // float a = r * TWOPI;

    // forward
    float wf = sense(i, 0);
    // left
    float wl = sense(i, iSenseAngle);
    // right
    float wr = sense(i, -iSenseAngle);

    if (wf <= wl || wf <= wr)
    {
        if (wf < wl && wf < wr)
            a += (rand * 2 * PI - 0.5) * 2 * iRotSpeed;
        else if (wr > wl)
            a -= rand * iRotSpeed * 2 * PI;
        else if (wl > wr)
            a += rand * iRotSpeed * 2 * PI;
    }
    
    vec2 vel = vec2(cos(a), sin(a));
    vec2 pos = src[i] + vel;

    if (pos.x < 0.0 || pos.y < 0.0 || 
        pos.x >= fRes.x || pos.y >= fRes.y ||
        pos.x == src[i].x || pos.y == src[i].y) 
    {
        pos.x = min(fRes.x - 1, max(0, pos.x));
        pos.y = min(fRes.y - 1, max(0, pos.y));

        a = rand * TWOPI;
    }

    src[i] = pos;
    rots[i] = mod(a, TWOPI);

    imageStore(iTex, ivec2(pos), vec4(1.0, 1.0, 1.0, 1.0));
}
