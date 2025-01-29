#version 430                                                                   
#define WAVES X                                                                
#define BLOB_COUNT X                                                           
#define CLUSTER_SIZE X                                                         
// DO NOT EDIT ABOVE

#define PI 3.1415f

layout(local_size_x = WAVES) in;
layout(binding = 3, rgba8) uniform image2D blobTex;
layout(std430, binding = 1) buffer blobsPosLayout
{
    vec2 src[BLOB_COUNT];
};
layout(std430, binding = 2) buffer blobsRotLayout
{
    float rots[BLOB_COUNT];
};

uniform int iFrame;
uniform ivec2 iResolution;
uniform float iTime;
uniform float iRotSpeed;
uniform int iSensorSize;
uniform float iSenseAngle;
uniform float iDstOffset;
uniform int iDensityThreshold;

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
    )) / 4294967295.0;
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

            sum += imageLoad(blobTex, ivec2(posx, posy)).r;
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

    // forward
    float wf = sense(i, 0);
    // left
    float wl = sense(i, iSenseAngle);
    // right
    float wr = sense(i, -iSenseAngle);

    a += (wf > iDensityThreshold) ? PI + ((rand - 0.5) * 2.0) * iSenseAngle
        : (wf < wl && wf < wr) ? ((rand - 0.5) * 2.0) * iRotSpeed * PI
                : wr > wl ? -rand * iRotSpeed * PI
                    : wl > wr ? rand * iRotSpeed * PI : 0;

    vec2 vel = vec2(cos(a), sin(a)) * 2.0;
    vec2 pos = src[i];
    vec2 npos = pos + vel;

    a = (npos.x < 0 || npos.y < 0 || npos.x >= fRes.x || npos.y >= fRes.y)
        ? rand * 2 * PI : a;
    npos.x = min(fRes.x, max(0, npos.x));
    npos.y = min(fRes.y, max(0, npos.y));

    for (float offx = -1; offx < 1; offx++)
    {
        for (float offy = -1; offy < 1; offy++)
        {
            float posx = min(fRes.x - 1, max(0, npos.x + offx));
            float posy = min(fRes.y - 1, max(0, npos.y + offy));
            imageStore(blobTex, ivec2(posx,posy), vec4(1.0, 1.0, 1.0, 1.0));
        }
    }

    src[i] = npos;
    rots[i] = mod(a, 2 * PI);
}
