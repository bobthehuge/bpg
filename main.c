#define _POSIX_C_SOURCE 199309L

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define SEED time(NULL)
// #define SEED 42

// #define BG_COLOR RAYWHITE
#define COLOR_INVERT(c) CLITERAL(Color){255-(c).r, 255-(c).g, 255-(c).b, (c).a}
#define SEMI_COLOR(c) CLITERAL(Color){20+(c).r, 20+(c).g, 20+(c).b, (c).a}
// #define BG_COLOR CLITERAL(Color){30, 30, 30, 255}
#define BG_COLOR BLACK
#define MG_COLOR SEMI_COLOR(BG_COLOR)
#define FG_COLOR COLOR_INVERT(BG_COLOR)
#define FADE_COLOR CLITERAL(Color){0, 0, 0, 2}

#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define TEX_WIDTH 2560
#define TEX_HEIGHT 1440
// #define TEX_WIDTH WIN_WIDTH
// #define TEX_HEIGHT WIN_HEIGHT

#define TEX_RECT CLITERAL(Rectangle){0,0,TEX_WIDTH,TEX_HEIGHT}
#define WIN_RECT CLITERAL(Rectangle){0,0,WIN_WIDTH,WIN_HEIGHT}
// #define WIN_CENTER CLITERAL(Vector2){TEX_WIDTH/2.0, TEX_HEIGHT/2.0}

#define BLOB_SIZE 2
// #define BLOB_COUNT 1000000
#define BLOB_COUNT 16384
// #define BLOB_COUNT 4096
// #define BLOB_COUNT 2048
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4

#define VEC2FMT "{ x:%f, y:%f }"
#define VEC2IFMT "{ x:%d, y:%d }"

#define LOCAL_SIZE 256
#define RGBA8 RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8

static int g_seed = 0;

float randf(void)
{
    return (float)rand()/(float)(RAND_MAX);;
}

void log_infos(void)
{
    TraceLog(LOG_INFO, "\tWindow size:\t%dx%d", WIN_WIDTH, WIN_HEIGHT);
    TraceLog(LOG_INFO, "\tTexture size:\t%dx%d", TEX_WIDTH, TEX_HEIGHT);
    TraceLog(LOG_INFO, "\tTotal cell:\t%d", BLOB_COUNT);
    TraceLog(LOG_INFO, "\tSeed:\t\t%d", g_seed);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");
    ToggleFullscreen();

    Image tmp_img = GenImageColor(TEX_WIDTH, TEX_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(tmp_img);
    UnloadImage(tmp_img);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);

    char *com_code = LoadFileText("compute.glsl");
    int com_shader = rlCompileShader(com_code, RL_COMPUTE_SHADER);
    int com_prog = rlLoadComputeShaderProgram(com_shader);
    UnloadFileText(com_code);

    char *dif_code = LoadFileText("diffuse.glsl");
    int dif_shader = rlCompileShader(dif_code, RL_COMPUTE_SHADER);
    int dif_prog = rlLoadComputeShaderProgram(dif_shader);
    UnloadFileText(dif_code);

    int com_res_loc = rlGetLocationUniform(com_prog, "iResolution");
    int com_frame_loc = rlGetLocationUniform(com_prog, "iFrame");
    int com_time_loc = rlGetLocationUniform(com_prog, "iTime");
    int com_tex_loc = rlGetLocationUniform(com_prog, "iTex");

    int dif_res_loc = rlGetLocationUniform(dif_prog, "iResolution");

    const int res[2] = {TEX_WIDTH, TEX_HEIGHT};

    Vector2 *blobs_pos = malloc(BLOB_COUNT * sizeof(Vector2));
    // Vector2 blobs_vel = {0};
    float *blobs_rot = malloc(BLOB_COUNT * sizeof(float));

    g_seed = SEED;

    srand(g_seed);

    SetTraceLogLevel(LOG_INFO);
    log_infos();
    SetTraceLogLevel(LOG_WARNING);

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        blobs_pos[i].x = randf() * TEX_WIDTH;
        blobs_pos[i].y = randf() * TEX_HEIGHT;
        blobs_rot[i] = randf();
    }

    int ssbo_pos = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(Vector2),
        // NULL,
        blobs_pos,
        RL_DYNAMIC_COPY
    );
    
    int ssbo_rot = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(float),
        // NULL,
        blobs_rot,
        RL_DYNAMIC_COPY
    );

    free(blobs_pos);
    free(blobs_rot);
  
    rlBindShaderBuffer(ssbo_pos, 1);
    rlBindShaderBuffer(ssbo_rot, 2);
    rlBindImageTexture(texture.id, 3, RGBA8, false);

    uint32_t frame_count = 0;
    uint32_t fps = 300;
    SetTargetFPS(fps);

    while (!WindowShouldClose())
    {
        float elapsed = GetTime();

        if (IsKeyPressed(KEY_UP) && fps < 1800)
        {
            fps += 60;
            SetTargetFPS(fps);
        }
        if (IsKeyPressed(KEY_DOWN) && fps > 0)
        {
            fps -= 60;
            SetTargetFPS(fps);
        }

        rlEnableShader(dif_prog);
            rlSetUniform(
                dif_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlComputeShaderDispatch(
                TEX_WIDTH / 16,
                TEX_HEIGHT / 16, 
                1
            );
        rlDisableShader();
        
        rlEnableShader(com_prog);
            rlSetUniform(
                com_frame_loc,
                &frame_count,
                SHADER_UNIFORM_INT,
                1
            );

            rlSetUniform(
                com_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlSetUniform(
                com_time_loc,
                &elapsed,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlComputeShaderDispatch(BLOB_COUNT / LOCAL_SIZE, 1, 1);
        rlDisableShader();

        BeginDrawing();
            ClearBackground(BLANK);
                DrawTexturePro(
                    texture,
                    TEX_RECT,
                    WIN_RECT,
                    Vector2Zero(),
                    0,
                    WHITE
                );
            DrawFPS(0, 0);
        EndDrawing();

        frame_count++;
    }

    SetTraceLogLevel(LOG_WARNING);
    
    rlUnloadShaderBuffer(ssbo_pos);
    rlUnloadShaderBuffer(ssbo_rot);
    rlUnloadShaderProgram(com_prog);

    CloseWindow();

    return 0;
}
