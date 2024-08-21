#define _POSIX_C_SOURCE 199309L

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SEED time(NULL)
// #define SEED 42

#define BTH_SIMPLEX_IMPLEMENTATION
#include "bth_simplex.h"

#define TWO_PI 6.28318530717958647692f
#define VANGLE (PI / 20)

// #define BG_COLOR RAYWHITE
#define COLOR_INVERT(c) CLITERAL(Color){255-(c).r, 255-(c).g, 255-(c).b, (c).a}
#define SEMI_COLOR(c) CLITERAL(Color){20+(c).r, 20+(c).g, 20+(c).b, (c).a}
// #define BG_COLOR CLITERAL(Color){30, 30, 30, 255}
#define BG_COLOR BLACK
#define MG_COLOR SEMI_COLOR(BG_COLOR)
#define FG_COLOR COLOR_INVERT(BG_COLOR)
#define FADE_COLOR CLITERAL(Color){0, 0, 0, 2}

#define MAG 0.005
#define VEL 1

#define SCALE 5
#define SCALE_FACTOR 0.7
#define SMOOTHER_SCALE SCALE_FACTOR / 10000

#define WIN_WIDTH 1200
#define WIN_HEIGHT 800
#define WIN_BOUNDS CLITERAL(Rectangle){0, 0, WIN_WIDTH, WIN_HEIGHT}

#define ROW_COUNT (WIN_HEIGHT / SCALE)
#define COL_COUNT (WIN_WIDTH / SCALE)

#define CELL_COUNT (ROW_COUNT * COL_COUNT)

#define OCTAVES 4

#define BLOB_SIZE 2
// #define BLOB_COUNT 4096
#define BLOB_COUNT 256
// #define BLOB_COUNT 4

#define VEC2FMT "{ x:%f, y:%f }"
#define VEC2IFMT "{ x:%d, y:%d }"

#define UNIFORM_DVEC2 (2*sizeof(double))

// #define DRAW_BLOB(vec2) DrawCircleV((vec2), BLOB_SIZE, FG_COLOR)
#define DRAW_BLOB(vec2) DrawPixelV((vec2), WHITE)

typedef struct Vector2i {
    int x;
    int y;
} Vector2i;

typedef struct Blob {
    Vector2i pos;
    Vector2i vel;
    // float acc;
    float rot;
    float foo;
} Blob;

struct simulation {
    int rows;
    int cols;
    float zoff;
    // Cell *cells;
    Blob *blobs;
};

static int g_seed = 0;

float randf(void)
{
    return (float)rand()/(float)(RAND_MAX);;
}

void log_infos(void)
{
    TraceLog(LOG_INFO, "\tWindow size:\t%dx%d", WIN_WIDTH, WIN_HEIGHT);
    TraceLog(LOG_INFO, "\tBoard size:\t%dx%d", COL_COUNT, ROW_COUNT);
    TraceLog(LOG_INFO, "\tCell size:\t%d", SCALE);
    TraceLog(LOG_INFO, "\tTotal cell:\t%d", CELL_COUNT);
    TraceLog(LOG_INFO, "\tSeed:\t\t%d", g_seed);
}

void sim_init_blobs(struct simulation *sim)
{
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        sim->blobs[i].pos.x = randf() * WIN_WIDTH;
        sim->blobs[i].pos.y = randf() * WIN_HEIGHT;
        sim->blobs[i].vel.x = VEL;
        sim->blobs[i].vel.y = VEL;
        sim->blobs[i].rot = randf();
    }
}

void print_pos(Vector2i pos[BLOB_COUNT])
{
    TraceLog(LOG_INFO, "Pos:");
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        TraceLog(LOG_INFO, "\t"VEC2IFMT, pos[i].x, pos[i].y);
    }
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    SetTargetFPS(120);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");

    RenderTexture2D tex0 = LoadRenderTexture(WIN_WIDTH, WIN_HEIGHT);

    SetTraceLogLevel(LOG_INFO);

    Shader disp = LoadShader(0, "shader.glsl");

    char *com_code = LoadFileText("compute.glsl");
    int com_shader = rlCompileShader(com_code, RL_COMPUTE_SHADER);
    int com_prog = rlLoadComputeShaderProgram(com_shader);
    UnloadFileText(com_code);

    int com_res_loc = rlGetLocationUniform(com_prog, "iResolution");
    int com_frame_loc = rlGetLocationUniform(com_prog, "iFrame");
    int com_time_loc = rlGetLocationUniform(com_prog, "iTime");

    int disp_res_loc = GetShaderLocation(disp, "iResolution");
    int disp_tex0_loc = GetShaderLocation(disp, "texture0");
    int disp_bpos_loc = GetShaderLocation(disp, "blobs_pos");
    int disp_frame_loc = GetShaderLocation(disp, "iFrame");
    int disp_time_loc = GetShaderLocation(disp, "iTime");

    const int res[2] = {WIN_WIDTH, WIN_HEIGHT};
    SetShaderValue(disp, disp_res_loc, &res, SHADER_UNIFORM_IVEC2);
    // SetShaderValue(disp, 0, 0, SHADER_UNIFORM_INT);
    SetShaderValueTexture(disp, disp_tex0_loc, tex0.texture);

    uint32_t frame_count = 0;
    uint32_t update_blob_frame = 100;
    uint32_t draw_frame = 100;

    Blob blobs[BLOB_COUNT] = {0};

    Vector2 blobs_pos[BLOB_COUNT] = {0};
    // Vector2 blobs_vel = {0};
    float blobs_rot[BLOB_COUNT] = {0};

    g_seed = SEED;

    struct simulation sim = {
        .rows = ROW_COUNT,
        .cols = COL_COUNT,
        .zoff = 0,
        .blobs = blobs
    };

    srand(g_seed);
    sim_init_blobs(&sim);

    log_infos();

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        blobs_pos[i].x = sim.blobs[i].pos.x;
        blobs_pos[i].y = sim.blobs[i].pos.y;
        // blobs_rot[i] = *(int *)&(sim.blobs[i].rot);
        blobs_rot[i] = sim.blobs[i].rot;
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
  
    while (!WindowShouldClose())
    {
        float elapsed = GetTime();

        rlBindShaderBuffer(ssbo_pos, 1);
        rlBindShaderBuffer(ssbo_rot, 2);

        SetShaderValue(
            disp,
            disp_frame_loc,
            &frame_count,
            SHADER_UNIFORM_INT
        );

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

            rlComputeShaderDispatch(BLOB_COUNT, 1, 1);
            // rlComputeShaderDispatch(1, 1, 1);
        rlDisableShader();

        SetShaderValue(
            disp,
            disp_time_loc,
            &elapsed,
            SHADER_UNIFORM_FLOAT
        );
        
        BeginTextureMode(tex0);
            BeginShaderMode(disp);
                DrawTexture(tex0.texture, 0, 0, WHITE);
            EndShaderMode();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLANK);
            DrawTexture(tex0.texture, 0, 0, WHITE);
            DrawFPS(0, 0);

            DrawText(TextFormat("%f", elapsed), 0, 20, 20, WHITE);
        EndDrawing();

        frame_count++;
    }

    SetTraceLogLevel(LOG_WARNING);
    UnloadRenderTexture(tex0);
    UnloadShader(disp);
    
    rlUnloadShaderBuffer(ssbo_pos);
    rlUnloadShaderBuffer(ssbo_rot);
    rlUnloadShaderProgram(com_prog);

    CloseWindow();

    return 0;
}
