#version 460

#define BLOB_COUNT 4096
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4
#define UINT_MAX 0xFFFFFFFFu
#define TWOPI 6.2831f

#define LOCAL_SIZE 256

layout (local_size_x = LOCAL_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 1) buffer blobsPosLayout {
    vec2 src[BLOB_COUNT];
};

layout(std430, binding = 2) buffer blobsRotLayout {
    float rots[BLOB_COUNT];
};

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

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (i >= BLOB_COUNT)
        return;
    
    vec2 fRes = vec2(iResolution);
    
    float r = rots[i];
    float a = r * TWOPI;
    
    vec2 vel = vec2(cos(a), sin(a));
    vec2 pos = src[i] + vel;

    if (pos.x <= 0.0 || pos.y <= 0.0 || 
        pos.x >= fRes.x - 1.0 || pos.y >= fRes.y - 1.0 ||
        pos.x == src[i].x || pos.y == src[i].y) 
    {
        pos.x = min(fRes.x - 1, max(0, pos.x));
        pos.y = min(fRes.y - 1, max(0, pos.y));

        rots[i] = randf(i);
    }

    src[i] = pos;
}
