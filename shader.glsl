#version 430

#define BLOB_COUNT 256
#define ERASER_WEIGHT 0.005

// #define EPS 0.000001f
// #define FLT_MAX 3.402823466e+38
// #define FLT_MIN 1.175494351e-38
// #define DBL_MAX 1.7976931348623158e+308
// #define DBL_MIN 2.2250738585072014e-308

const float kernel = 10.0;
const float weight = 1.0;

uniform sampler2D texture0;

// layout(std430, binding = 1) readonly buffer blobsPosSrcLayout
// {
//     ivec2 src[BLOB_COUNT];
// };

layout(std430, binding = 2) readonly buffer blobsPosDstLayout
{
    ivec2 dst[BLOB_COUNT];
};

uniform int iFrame;
uniform vec2 iResolution;

in vec2 fragTexCoord;

out vec4 finalColor;

bool ivec2eq(ivec2 a, ivec2 b)
{
    return a.x == b.x && a.y == b.y;
}

void main()
{
    uint frame = uint(iFrame);

    vec4 src_col;

    src_col = texture(texture0, fragTexCoord);

    if (src_col.xyz != vec3(0.0))
        src_col.xyz -= vec3(ERASER_WEIGHT);
    
    ivec2 pos = ivec2(fragTexCoord * iResolution);
    bool even_frame = frame % 2u == 0;

    int i;
    for (i = 0; i < BLOB_COUNT; i++)
    {
        if (ivec2eq(pos, dst[i]))
        {
            src_col = vec4(1.0, 1.0, 1.0, 1.0);
            i = BLOB_COUNT;
        }
    }

    finalColor = src_col;
}
