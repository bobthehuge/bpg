#version 460

#define BLOB_COUNT 4096
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4

// #define ERASER_WEIGHT 0.005
#define ERASER_WEIGHT 0.01
#define TWOPI 6.2831f

uniform sampler2D texture0;

layout(std430, binding = 1) readonly buffer blobsPosSrcLayout
{
    vec2 src[BLOB_COUNT];
};

// uniform int iFrame;
uniform ivec2 iResolution;
// uniform float iTime;

in vec2 fragTexCoord;
out vec4 finalColor;

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
