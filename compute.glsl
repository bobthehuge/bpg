#version 430

#define BLOB_COUNT 256
#define EPS 0.000001f
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308

// const float kernel = 10.0;
// const float weight = 1.0;

// uniform sampler2D texture0;
// uniform sampler2D texture1;

// uniform vec2 blobs_pos[BLOB_COUNT];
// uniform vec2 blobs_vel[BLOB_COUNT];
// uniform float blobs_rot[BLOB_COUNT];

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 1) readonly buffer blobsPosSrcLayout {
    ivec2 src[BLOB_COUNT];
};

layout(std430, binding = 2) writeonly buffer blobsPosDstLayout {
    ivec2 dst[BLOB_COUNT];
};

uniform int iFrame;
uniform vec2 iResolution;

bool feq(float a, float b)
{
    return a >= b && a <= b + 1;
    // WTF THIS ISN'T WORKING ????????????
    // return abs(a - b) <= EPS;
}

bool vec2eq(vec2 a, vec2 b)
{
    return feq(a.x, b.x) && feq(a.y, b.y);
}

bool ivec2eq(ivec2 a, ivec2 b)
{
    return a.x == b.x && a.y == b.y;
}

void main()
{
    // ivec2 pos = ivec2(fragTexCoord * iResolution);
    // int iFrame = 0;
    uint frame = uint(iFrame);
    bool even_frame = frame % 2u == 0;

    ivec2 vel = ivec2(1, 0);

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        dst[i] = src[i] + vel;
    }
}
