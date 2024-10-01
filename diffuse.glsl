#version 430

// #define LOCAL_SIZE 256

layout (local_size_x = 32) in;

layout(binding = 3, rgba8) uniform image2D iTex;

uniform ivec2 iResolution;

void main()
{
    uvec2 id = uvec2(
        gl_GlobalInvocationID.x % iResolution.x, 
        gl_GlobalInvocationID.x / iResolution.x
    );

    if (id.x >= iResolution.x || id.y >= iResolution.y)
        return;

    vec4 col = imageLoad(iTex, ivec2(id));
    vec4 sum = vec4(0.0);

    for (int offx = -1; offx <= 1; offx++)
    {
        for (int offy = -1; offy <= 1; offy++)
        {
            ivec2 pos = ivec2(id) + ivec2(offx, offy);

            if (pos.x >= 0 && pos.x < iResolution.x && 
                pos.y >= 0 && pos.y < iResolution.y
            ) {
                sum += imageLoad(iTex, pos);
            }
        }
    }

    vec4 dif = mix(col, sum / 9.0, 1.0); //lerp
    vec4 final_dif = max(vec4(0), dif - 0.005);
    
    imageStore(iTex, ivec2(id), final_dif);
    // pos = x * width + y
    // vec = pos % width
}
