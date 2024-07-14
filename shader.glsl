#version 330

const float kernel = 10.0;
const float weight = 1.0;

uniform sampler2D texture0;
uniform sampler2D texture1;

struct Blob
{
    vec2 pos;
    vec2 vel;
    float rot;
};

layout (std140) uniform BlobData
{
    Blob blobs[256];
};

uniform int s_frame_count;
// uniform vec2 iResolution;

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

void main()
{
    uint frame_count = uint(s_frame_count);
    vec4 col;

    if (frame_count % 2u == 0u)
    {
        col = texture(texture0, fragTexCoord);
    }
    else
    {
        col = texture(texture1, fragTexCoord);
    }
    
    // vec4 col = texture(texture0, fragTexCoord);

    finalColor = col;
}
