#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <time.h>

#define SEED time(NULL)

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

// #define DRAW_BLOB(vec2) DrawCircleV((vec2), BLOB_SIZE, FG_COLOR)
#define DRAW_BLOB(vec2) DrawPixelV((vec2), WHITE)

typedef struct Blob {
    Vector2 pos;
    Vector2 vel;
    // float acc;
    float rot;
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
        sim->blobs[i].vel = CLITERAL(Vector2){ VEL, VEL };
        sim->blobs[i].rot = randf();
    }
}

void sim_update_blobs(struct simulation *sim)
{
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        Blob *blob = &sim->blobs[i];
        Vector2 *bpos = &blob->pos;
        Vector2 *vel = &blob->vel;

        float x = floorf(bpos->x / SCALE) * SCALE_FACTOR;
        float y = floorf(bpos->y / SCALE) * SCALE_FACTOR;
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
            bpos->x = nx;

        // if (!isnan(ny))
            bpos->y = ny;
        // bpos->x = xclamp(nx);
        // bpos->y = yclamp(ny);

        sim->zoff += SMOOTHER_SCALE;
    }
}

void sim_draw_blobs(struct simulation *sim)
{
    for (int i = 0; i < BLOB_COUNT; i++)
    {
        DRAW_BLOB(sim->blobs[i].pos);
    }
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    // SetTargetFPS(60);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");

    RenderTexture2D tex0 = LoadRenderTexture(WIN_WIDTH, WIN_HEIGHT);
    RenderTexture2D tex1 = LoadRenderTexture(WIN_WIDTH, WIN_HEIGHT);

    SetTraceLogLevel(LOG_INFO);
    // Texture2D tex2 = LoadTexture("sample.png");

    Shader shader = LoadShader(0, "shader.glsl");

    // int resLoc = GetShaderLocation(shader, "iResolution");
    // float res[2] = {WIN_WIDTH, WIN_HEIGHT};
    // SetShaderValue(shader, resLoc, res, SHADER_UNIFORM_VEC2);

    int tex0Loc = GetShaderLocation(shader, "texture0");
    int tex1Loc = GetShaderLocation(shader, "texture1");
    int blobsLoc = GetShaderLocation(shader, "blobs");

    int frameLoc = GetShaderLocation(shader, "s_frame_count");
    SetShaderValue(shader, 0, 0, SHADER_UNIFORM_INT);
   
    uint32_t frame_count = 0;
    uint32_t update_blob_frame = 100;
    uint32_t draw_frame = 100;
    Blob blobs[BLOB_COUNT] = {0};

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

    BeginTextureMode(tex0);
        ClearBackground(BLACK);
        sim_draw_blobs(&sim);
    EndTextureMode();

    BeginTextureMode(tex1);
        // DrawRectangle(50, 50, 200, 200, YELLOW);
        ClearBackground(BLACK);
    EndTextureMode();

    RenderTexture2D *src;
    RenderTexture2D *dst;

    while (!WindowShouldClose())
    {
        // if (!(frame_count % update_blob_frame))
        // {
        //     sim_update_blobs(&sim);
        // }
        
        SetShaderValue(shader, frameLoc, &frame_count, SHADER_UNIFORM_INT);

        // when frame_count is odd, draw on tex0
        if (frame_count % 2)
        {
            src = &tex1;
            dst = &tex0;
        }
        // when frame_count is even, draw on tex1
        else
        {
            src = &tex0;
            dst = &tex1;
        }
        
        if (!(frame_count % draw_frame))
        {
            BeginTextureMode(*dst);
                DrawTexture(src->texture, 0, 0, WHITE);
            EndTextureMode();

            BeginDrawing();
                BeginShaderMode(shader);
                    SetShaderValueTexture(shader, tex0Loc, tex0.texture);
                    SetShaderValueTexture(shader, tex1Loc, tex1.texture);

                    ClearBackground(BLACK);
                    DrawTexture(dst->texture, 0, 0, WHITE);
                EndShaderMode();
            EndDrawing();
        }

        // DrawFPS(0, 0);
        frame_count++;
    }

    SetTraceLogLevel(LOG_WARNING);
    UnloadRenderTexture(tex0);
    UnloadRenderTexture(tex1);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}
