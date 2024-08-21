#version 430

#define BLOB_COUNT 256
// #define BLOB_COUNT 4
// #define EPS 0.000001f
#define EPS 0.0005f
// #define EPS 0.5f
#define ERASER_WEIGHT 0.005
#define TWOPI 6.2831f
#define PI 3.1416f

#define UINT_MAX 0xFFFFFFFFu
#define FLT_MAX 3.402823466e+38

const float kernel = 10.0;
const float weight = 1.0;

uniform sampler2D texture0;
precision highp float;

layout(std430, binding = 1) readonly buffer blobsPosSrcLayout
{
    vec2 src[BLOB_COUNT];
};

// layout(std430, binding = 2) readonly buffer blobsPosDstLayout
// {
//     uvec2 dst[BLOB_COUNT];
// };

uniform int iFrame;
uniform ivec2 iResolution;
uniform float iTime;

in vec2 fragTexCoord;
out vec4 finalColor;

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
    // float r = normalize(randf(uvec3(src[id].xy, uint(id))));
    float r = randf(uvec3(src[id].xy, uint(id)));
    r = isnan(r) ? 0.0 : isinf(r) ? 1.0 : r;
    return r;
}

void main()
{
    vec4 src_col;

    vec2 invpos = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec2 rpos = vec2(fragTexCoord * iResolution);
    ivec2 pos = ivec2(rpos);

    src_col = texture(texture0, invpos);

    if (src_col.xyz != vec3(0.0))
        src_col.xyz -= vec3(ERASER_WEIGHT);

    uint i;
    for (i = 0; i < BLOB_COUNT; i++)
    {
        ivec2 bpos = ivec2(src[i]);

        if (pos.x == bpos.x && pos.y == bpos.y)
        {
            src_col = vec4(1.0, 1.0, 1.0, 1.0);
            i = BLOB_COUNT;
        }
    }

    finalColor = src_col;
}
