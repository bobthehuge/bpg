#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
// #include <time.h>

// #define SEED time(NULL)
#define SEED 42

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
// #define BLOB_COUNT 256
#define BLOB_COUNT 4

#define VEC2FMT "{ x:%f, y:%f }"
#define VEC2IFMT "{ x:%d, y:%d }"

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

float xclamp(float x)
{
    if (x > WIN_WIDTH)
        return 0;

    if (x < 0)
        return WIN_WIDTH;

    return x;
}

float yclamp(float y)
{
    if (y > WIN_HEIGHT)
        return 0;

    if (y < 0)
        return WIN_HEIGHT;

    return y;
}

// transform x from [-1; 1] to [0; 1]
float normalize(float x)
{
    return (x + 1) / 2;
}

void log_infos(void)
{
    TraceLog(LOG_INFO, "\tWindow size:\t%dx%d", WIN_WIDTH, WIN_HEIGHT);
    TraceLog(LOG_INFO, "\tBoard size:\t%dx%d", COL_COUNT, ROW_COUNT);
    TraceLog(LOG_INFO, "\tCell size:\t%d", SCALE);
    TraceLog(LOG_INFO, "\tTotal cell:\t%d", CELL_COUNT);
    TraceLog(LOG_INFO, "\tSeed:\t\t%d", g_seed);
}

Vector2 Vector2Rot(Vector2 x, Vector2 org, float angle)
{
    Vector2 res = Vector2Subtract(x, org);
    res = Vector2Rotate(res, angle);
    res = Vector2Add(res, org);

    return res;
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

void sim_update_blobs(struct simulation *sim)
{
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        Blob *blob = &sim->blobs[i];
        Vector2i *bpos = &blob->pos;
        Vector2i *vel = &blob->vel;

        float x = floor((float)bpos->x / SCALE) * SCALE_FACTOR;
        float y = floor((float)bpos->y / SCALE) * SCALE_FACTOR;
        float z = sim->zoff;

        float a = perlin_fbm3d(OCTAVES, x, y, z) * PI;

        // Cell *cell = &CELL_AT(sim, x, y);

        // a * MAG -> [0; TWO_PI * MAG]
        // rot -> [0; TWO_PI]
        // float a = fmodf(cell->a * MAG, TWO_PI);
        // blob->rot = fmodf(blob->rot + cell->a * MAG, PI);
        blob->rot = fmodf(blob->rot + a * MAG, PI);

        if (bpos->x >= WIN_WIDTH - 1 || bpos->x <= 0)
        {
            vel->x *= -1;
        }

        if (bpos->y >= WIN_HEIGHT - 1 || bpos->y <= 0)
            vel->y *= -1;

        float cosx = cosf(blob->rot);
        float sinx = sinf(blob->rot);

        cosx = isnan(cosx) ? 1 : cosx;
        sinx = isnan(sinx) ? 0 : sinx;

        float nx = bpos->x + cosx * vel->x;
        float ny = bpos->y + sinx * vel->y;

        // if (!isnan(nx))
            bpos->x = (int)nx;

        // if (!isnan(ny))
            bpos->y = (int)ny;
        // bpos->x = xclamp(nx);
        // bpos->y = yclamp(ny);

        sim->zoff += SMOOTHER_SCALE;
    }
}

void sim_draw_blobs(struct simulation *sim)
{
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        // DRAW_BLOB(sim->blobs[i].pos);
        Vector2i *v = &sim->blobs[i].pos;
        Vector2 fv = {v->x, v->y};
        DRAW_BLOB(fv);
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
    SetTargetFPS(60);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");

    RenderTexture2D tex0 = LoadRenderTexture(WIN_WIDTH, WIN_HEIGHT);

    SetTraceLogLevel(LOG_INFO);

    Shader shader = LoadShader(0, "shader.glsl");

    char *com_code = LoadFileText("compute.glsl");
    int com_shader = rlCompileShader(com_code, RL_COMPUTE_SHADER);
    int com_prog = rlLoadComputeShaderProgram(com_shader);
    UnloadFileText(com_code);

    int resLoc = GetShaderLocation(shader, "iResolution");
    float res[2] = {WIN_WIDTH, WIN_HEIGHT};
    SetShaderValue(shader, resLoc, res, SHADER_UNIFORM_VEC2);

    int comResLoc = rlGetLocationUniform(com_shader, "iResolution");
    int comFrameLoc = rlGetLocationUniform(com_shader, "iFrame");

    int tex0_loc = GetShaderLocation(shader, "texture0");
    int blobs_pos_loc = GetShaderLocation(shader, "blobs_pos");

    int frame_loc = GetShaderLocation(shader, "iFrame");
    SetShaderValue(shader, 0, 0, SHADER_UNIFORM_INT);

    uint32_t frame_count = 0;
    uint32_t update_blob_frame = 100;
    uint32_t draw_frame = 100;

    Blob blobs[BLOB_COUNT] = {0};

    Vector2i blobs_pos[BLOB_COUNT] = {0};
    // Vector2 blobs_vel = {0};
    // float blobs_rot[BLOB_COUNT] = {0};

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
        blobs_pos[i] = sim.blobs[i].pos;
    }

    int ssbo_pos0 = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(Vector2i),
        NULL,
        // blobs_pos,
        RL_DYNAMIC_COPY
    );
    
    int ssbo_pos1 = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(Vector2i),
        // NULL,
        blobs_pos,
        RL_DYNAMIC_COPY
    );
    
    // int ssbo_rot1 = rlLoadShaderBuffer(
    //     BLOB_COUNT * sizeof(float),
    //     // NULL,
    //     blobs_rot,
    //     RL_DYNAMIC_COPY
    // );
  
    SetShaderValueTexture(shader, tex0_loc, tex0.texture);
    // SetShaderValueTexture(shader, tex1_loc, tex1.texture);

    // rlBindShaderBuffer(ssbo_pos0, 1);
    // rlBindShaderBuffer(ssbo_pos1, 2);

    int ssbo_src = ssbo_pos1;
    int ssbo_dst = ssbo_pos0;

    while (!WindowShouldClose())
    {
        rlBindShaderBuffer(ssbo_src, 1);
        rlBindShaderBuffer(ssbo_dst, 2);

        SetShaderValue(
            shader,
            frame_loc,
            &frame_count,
            SHADER_UNIFORM_INT
        );

        rlEnableShader(com_prog);
            rlSetUniform(
                comFrameLoc,
                &frame_count,
                SHADER_UNIFORM_INT,
                1
            );

            rlSetUniform(comResLoc, res, SHADER_UNIFORM_VEC2, 1);
            rlComputeShaderDispatch(BLOB_COUNT, 1, 1);
        rlDisableShader();

        BeginTextureMode(tex0);
            BeginShaderMode(shader);
                DrawTexture(tex0.texture, 0, 0, WHITE);
            EndShaderMode();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLANK);

            DrawTexture(tex0.texture, 0, 0, WHITE);
            
            DrawFPS(0, 0);
        EndDrawing();

        ssbo_src ^= ssbo_dst;
        ssbo_dst ^= ssbo_src;
        ssbo_src ^= ssbo_dst;

        frame_count++;
    }

    SetTraceLogLevel(LOG_WARNING);
    UnloadRenderTexture(tex0);
    UnloadShader(shader);

    rlUnloadShaderBuffer(ssbo_pos0);
    rlUnloadShaderBuffer(ssbo_pos1);
    rlUnloadShaderProgram(com_prog);

    CloseWindow();

    return 0;
}
