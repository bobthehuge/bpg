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

#define BLOB_COUNT 1000000
// #define BLOB_COUNT 100000
// #define BLOB_COUNT 4096
// #define BLOB_COUNT 2048
// #define BLOB_COUNT 256
// #define BLOB_COUNT 4

#define VEC2FMT "{ x:%f, y:%f }"
#define VEC2IFMT "{ x:%d, y:%d }"

#define LOCAL_SIZE 256
#define RGBA8 RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8

#define MENU_SHOW 0b00000001
#define MENU_LOCK 0b00000010

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

#define PARAM_COUNT 5
struct GUI
{
    char state;
    uint32_t selected;
    uint32_t target_fps;
    float rotation_speed;
    uint32_t sensor_size;
    uint32_t sense_angle;
    float angle_offset;
};

static struct GUI menu = {
    .state = 0,
    .selected = 0,
    .target_fps = 300,
    .rotation_speed = 0.8,
    .sensor_size = 2,
    .sense_angle = 40,
    .angle_offset = 20,
};

void draw_menu(void)
{
    if ((menu.state & (MENU_SHOW | MENU_LOCK)) == 0 )
        return;

    DrawRectangle(0, 0, 300, WIN_HEIGHT, (Color){ 0, 0, 0, 180 });

    DrawText(TextFormat("%d FPS", GetFPS()), 10, 0, 20, GREEN);

    if (menu.selected > 0)
        DrawCircle(5, 25 + 20 * menu.selected - 10, 3, RED);

    DrawText(
        TextFormat("targeted FPS: %d", menu.target_fps), 10, 25, 20, GREEN
    );
    DrawText(
        TextFormat("rotation speed: %f", menu.rotation_speed),
        10,
        45,
        20,
        GREEN
    );
    DrawText(
        TextFormat("sensor size: %d", menu.sensor_size),
        10,
        65,
        20,
        GREEN
    );
    DrawText(
        TextFormat("sense angle: %d deg", menu.sense_angle),
        10,
        85,
        20,
        GREEN
    );
    DrawText(
        TextFormat("angle offset: %f", menu.angle_offset),
        10,
        105,
        20,
        GREEN
    );
}

void menu_incr_param(void)
{
    switch (menu.selected)
    {
    case 1:
        if (menu.target_fps < 3600)
        {
            menu.target_fps += 60;
            SetTargetFPS(menu.target_fps);
        }
        break;
    case 2:
        if (menu.rotation_speed)
            menu.rotation_speed += 0.1;
        break;
    case 3:
        if (menu.sensor_size < 20)
            menu.sensor_size++;
        break;
    case 4:
        if (menu.sense_angle < 180)
            menu.sense_angle += 10;
        break;
    case 5:
        if (menu.angle_offset < 50)
            menu.angle_offset += 1;
        break;
    default:
        break;
    }
}

void menu_decr_param(void)
{
    switch (menu.selected)
    {
    case 1:
        if (menu.target_fps > 60)
        {
            menu.target_fps -= 60;
            SetTargetFPS(menu.target_fps);
        }
        break;
    case 2:
        if (menu.rotation_speed > 0)
            menu.rotation_speed -= 0.1;
        break;
    case 3:
        if (menu.sensor_size > 1)
            menu.sensor_size--;
        break;
    case 4:
        if (menu.sense_angle > 0)
            menu.sense_angle -= 10;
        break;
    case 5:
        if (menu.angle_offset > -50)
            menu.angle_offset -= 1;
        break;
    default:
        break;
    }
}

void menu_reset_params(void)
{
    menu.target_fps = 300;
    menu.rotation_speed = 0.5;
    menu.sensor_size = 3;
}

void menu_controls(void)
{
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R))
    {
        menu_reset_params();
        return;
    }

    if ((menu.state & (MENU_SHOW | MENU_LOCK)) != MENU_SHOW)
        return;

    if (IsKeyPressed(KEY_UP) && menu.selected > 0)
        menu.selected--;
    if (IsKeyPressed(KEY_DOWN) && menu.selected < PARAM_COUNT)
        menu.selected++;

    if (IsKeyPressed(KEY_RIGHT))
        menu_incr_param();
    if (IsKeyPressed(KEY_LEFT))
        menu_decr_param();
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
    int com_time_loc = rlGetLocationUniform(com_prog, "iTime");
    int com_speed_loc = rlGetLocationUniform(com_prog, "iRotSpeed");
    int com_size_loc = rlGetLocationUniform(com_prog, "iSensorSize");
    int com_angle_loc = rlGetLocationUniform(com_prog, "iSenseAngle");
    int com_dstoff_loc = rlGetLocationUniform(com_prog, "iDstOffset");

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
        blobs_rot[i] = randf() * 2 * PI;
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

    SetTargetFPS(menu.target_fps);

    while (!WindowShouldClose())
    {
        float elapsed = GetTime();
        
        if (IsKeyPressed(KEY_F2) && (menu.state & MENU_LOCK) == 0)
        {
            menu.state ^= MENU_SHOW;
        }

        menu_controls();

        rlEnableShader(dif_prog);
            rlSetUniform(
                dif_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlComputeShaderDispatch(
                TEX_WIDTH * TEX_HEIGHT / 32,
                1,
                1
            );
        rlDisableShader();
        
        rlEnableShader(com_prog);
            rlSetUniform(
                com_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlSetUniform(
                com_speed_loc,
                &menu.rotation_speed,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                com_time_loc,
                &elapsed,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                com_size_loc,
                &menu.sensor_size,
                SHADER_UNIFORM_INT,
                1
            );

            float angle = (float)menu.sense_angle * PI / 180;
            rlSetUniform(
                com_angle_loc,
                &angle,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                com_dstoff_loc,
                &menu.angle_offset,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlComputeShaderDispatch(BLOB_COUNT / 32, 1, 1);
        rlDisableShader();

        BeginDrawing();
            ClearBackground(BLACK);
                DrawTexturePro(
                    texture,
                    TEX_RECT,
                    WIN_RECT,
                    Vector2Zero(),
                    0,
                    WHITE
                );
            draw_menu();
        EndDrawing();
    }

    SetTraceLogLevel(LOG_WARNING);
    
    rlUnloadShaderBuffer(ssbo_pos);
    rlUnloadShaderBuffer(ssbo_rot);
    rlUnloadShaderProgram(com_prog);

    CloseWindow();

    return 0;
}
