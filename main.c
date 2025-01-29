#include <math.h>
// #define _POSIX_C_SOURCE 199309L

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define XLEN(s) sizeof(STR(s)) - 1
#define XSTR(s) STR(s)
#define STR(s) #s
#define LINESIZE 80

#define COLOR_INVERT(c) CLITERAL(Color){255-(c).r, 255-(c).g, 255-(c).b, (c).a}
#define SEMI_COLOR(c) CLITERAL(Color){20+(c).r, 20+(c).g, 20+(c).b, (c).a}
#define BG_COLOR BLACK
#define MG_COLOR SEMI_COLOR(BG_COLOR)
#define FG_COLOR COLOR_INVERT(BG_COLOR)
#define FADE_COLOR CLITERAL(Color){0, 0, 0, 2}

#define WAVES 32

#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define TEX_WIDTH 2560
#define TEX_HEIGHT 1440

#define TEX_RECT CLITERAL(Rectangle){0,0,TEX_WIDTH,TEX_HEIGHT}
#define TEX_CENTER CLITERAL(Vector2){TEX_WIDTH/2.0,TEX_HEIGHT/2.0}
#define WIN_RECT CLITERAL(Rectangle){0,0,WIN_WIDTH,WIN_HEIGHT}
// #define WIN_CENTER CLITERAL(Vector2){TEX_WIDTH/2.0, TEX_HEIGHT/2.0}

// #define BLOB_COUNT 1000000
#define BLOB_COUNT 100000
// #define BLOB_COUNT 32768

#define VEC2FMT "{ x:%f, y:%f }"
#define VEC2IFMT "{ x:%d, y:%d }"
// #define SPAWN_RADIUS TEX_HEIGHT/2
#define SPAWN_RADIUS 300

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

#define PARAM_COUNT 7
struct settings
{
    char state;
    float cooldown;
    uint32_t selected;
    uint32_t target_fps;
    float rotation_speed;
    uint32_t sensor_size;
    uint32_t sense_angle;
    float angle_offset;
    float decay_rate;
    uint32_t density_threshold;
};

static struct settings settings = {
    .state = 0,
    .cooldown = 0,
    .selected = 0,
    .target_fps = 1080,
    .rotation_speed = 0.1,
    .sensor_size = 2,
    .sense_angle = 20,
    .angle_offset = 20,
    .decay_rate = 0.050,
    .density_threshold = 16,
};

void draw_settings(void)
{
    if ((settings.state & (MENU_SHOW | MENU_LOCK)) == 0 )
        return;

    DrawRectangle(0, 0, 300, WIN_HEIGHT, (Color){ 0, 0, 0, 180 });

    DrawText(TextFormat("%d FPS", GetFPS()), 10, 0, 20, GREEN);

    if (settings.selected > 0)
        DrawCircle(5, 25 + 20 * settings.selected - 10, 3, RED);

    DrawText(
        TextFormat("targeted FPS: %d", settings.target_fps), 10, 25, 20, GREEN
    );
    DrawText(
        TextFormat("rotation speed: %f", settings.rotation_speed),
        10,
        45,
        20,
        GREEN
    );
    DrawText(
        TextFormat("sensor size: %d", settings.sensor_size),
        10,
        65,
        20,
        GREEN
    );
    DrawText(
        TextFormat("sense angle: %d deg", settings.sense_angle),
        10,
        85,
        20,
        GREEN
    );
    DrawText(
        TextFormat("angle offset: %f", settings.angle_offset),
        10,
        105,
        20,
        GREEN
    );
    DrawText(
        TextFormat("decay rate: %f", settings.decay_rate),
        10,
        125,
        20,
        GREEN
    );
    DrawText(
        TextFormat("max density: %u", settings.density_threshold),
        10,
        145,
        20,
        GREEN
    );
}

void settings_incr_param(void)
{
    switch (settings.selected)
    {
    case 1:
        if (settings.target_fps < 3600)
        {
            settings.target_fps += 60;
            SetTargetFPS(settings.target_fps);
        }
        break;
    case 2:
        if (settings.rotation_speed)
            settings.rotation_speed += 0.001;
        break;
    case 3:
        if (settings.sensor_size < 20)
            settings.sensor_size++;
        break;
    case 4:
        if (settings.sense_angle < 180)
            settings.sense_angle += 1;
        break;
    case 5:
        if (settings.angle_offset < 60)
            settings.angle_offset += 1;
        break;
    case 6:
        if (settings.decay_rate < 1.0)
            settings.decay_rate += 0.001;
        break;
    case 7:
        if (settings.density_threshold < UINT32_MAX)
            settings.density_threshold += 1;
        break;
    default:
        break;
    }
}

void settings_decr_param(void)
{
    switch (settings.selected)
    {
    case 1:
        if (settings.target_fps > 60)
        {
            settings.target_fps -= 60;
            SetTargetFPS(settings.target_fps);
        }
        break;
    case 2:
        if (settings.rotation_speed > 0)
            settings.rotation_speed -= 0.001;
        break;
    case 3:
        if (settings.sensor_size > 1)
            settings.sensor_size--;
        break;
    case 4:
        if (settings.sense_angle > 0)
            settings.sense_angle -= 1;
        break;
    case 5:
        if (settings.angle_offset > -60)
            settings.angle_offset -= 1;
        break;
    case 6:
        if (settings.decay_rate > 0.0)
            settings.decay_rate -= 0.001;
        break;
    case 7:
        if (settings.density_threshold > 0)
            settings.density_threshold -= 1;
        break;
    default:
        break;
    }
}

void settings_reset_params(void)
{
    settings.target_fps = 1080;
    settings.rotation_speed = 0.1;
    settings.sensor_size = 2;
    settings.sense_angle = 20;
    settings.angle_offset = 20;
    settings.decay_rate = 0.005;
    settings.density_threshold = 20;
}

void settings_controls(void)
{
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R))
    {
        settings_reset_params();
        return;
    }

    if ((settings.state & (MENU_SHOW | MENU_LOCK)) != MENU_SHOW)
        return;

    if (IsKeyPressed(KEY_UP) && settings.selected > 1)
        settings.selected--;
    if (IsKeyPressed(KEY_DOWN) && settings.selected < PARAM_COUNT)
        settings.selected++;

    if (IsKeyDown(KEY_LEFT_ALT) && settings.cooldown > 0.1)
    {
        settings.cooldown = 0;

        if (IsKeyDown(KEY_RIGHT))
            settings_incr_param();
        if (IsKeyDown(KEY_LEFT))
            settings_decr_param();
    } 
    else
    {
        if (IsKeyPressed(KEY_RIGHT))
            settings_incr_param();
        if (IsKeyPressed(KEY_LEFT))
            settings_decr_param();
    }
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");
    ToggleFullscreen();

    Image tmp_img = GenImageColor(TEX_WIDTH, TEX_HEIGHT, BLACK);
    Texture2D blob_map = LoadTextureFromImage(tmp_img);
    SetTextureFilter(blob_map, TEXTURE_FILTER_BILINEAR);
    UnloadImage(tmp_img);

    char *part_code = LoadFileText("particle.glsl");
    memcpy(
        part_code + LINESIZE + sizeof("#define WAVES"),
        XSTR(WAVES),
        XLEN(WAVES)
    );
    memcpy(
        part_code + 2*LINESIZE + sizeof("#define BLOB_COUNT"),
        XSTR(BLOB_COUNT),
        XLEN(BLOB_COUNT)
    );
    memcpy(
        part_code + 3*LINESIZE + sizeof("#define CLUSTER_SIZE"),
        XSTR(CLUSTER_SIZE),
        XLEN(CLUSTER_SIZE)
    );
    int part_shader = rlCompileShader(part_code, RL_COMPUTE_SHADER);
    int part_prog = rlLoadComputeShaderProgram(part_shader);
    UnloadFileText(part_code);

    char *dif_code = LoadFileText("diffuse.glsl");
    memcpy(
        dif_code + LINESIZE + sizeof("#define WAVES"),
        XSTR(WAVES),
        XLEN(WAVES)
    );
    int dif_shader = rlCompileShader(dif_code, RL_COMPUTE_SHADER);
    int dif_prog = rlLoadComputeShaderProgram(dif_shader);
    UnloadFileText(dif_code);

    int part_res_loc = rlGetLocationUniform(part_prog, "iResolution");
    int part_time_loc = rlGetLocationUniform(part_prog, "iTime");
    int part_speed_loc = rlGetLocationUniform(part_prog, "iRotSpeed");
    int part_size_loc = rlGetLocationUniform(part_prog, "iSensorSize");
    int part_angle_loc = rlGetLocationUniform(part_prog, "iSenseAngle");
    int part_dstoff_loc = rlGetLocationUniform(part_prog, "iDstOffset");
    int part_thresh_loc = rlGetLocationUniform(part_prog, "iDensityThreshold");

    int dif_res_loc = rlGetLocationUniform(dif_prog, "iResolution");
    int dif_dec_loc = rlGetLocationUniform(dif_prog, "iDecay");

    const int res[2] = {TEX_WIDTH, TEX_HEIGHT};

    Vector2 *blobs_pos = malloc(BLOB_COUNT * sizeof(Vector2));
    float *blobs_rot = malloc(BLOB_COUNT * sizeof(float));

    struct timeval d;
    gettimeofday(&d, NULL);

    g_seed = (d.tv_sec * 100 + d.tv_usec / 100) * getpid();
    srand(g_seed);

    SetTraceLogLevel(LOG_INFO);
    log_infos();
    SetTraceLogLevel(LOG_WARNING);

    for (int i = 0; i < BLOB_COUNT; i++)
    {
        float theta = randf() * 2 * PI;
        float r = sqrtf(randf());
        Vector2 pos = {
            r * cos(theta) * (float)SPAWN_RADIUS + TEX_WIDTH/2.0,
            r * sin(theta) * (float)SPAWN_RADIUS + TEX_HEIGHT/2.0,
        };

        blobs_pos[i] = pos;
        Vector2 org = Vector2Normalize(Vector2Subtract(TEX_CENTER, pos));
        blobs_rot[i] = randf() * 2 * PI;
    }

    int ssbo_pos = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(Vector2),
        blobs_pos,
        RL_DYNAMIC_COPY
    );
    
    int ssbo_rot = rlLoadShaderBuffer(
        BLOB_COUNT * sizeof(float),
        blobs_rot,
        RL_DYNAMIC_COPY
    );

    free(blobs_pos);
    free(blobs_rot);
  
    rlBindShaderBuffer(ssbo_pos, 1);
    rlBindShaderBuffer(ssbo_rot, 2);
    rlBindImageTexture(blob_map.id, 3, RGBA8, false);

    SetTargetFPS(settings.target_fps);

    while (!WindowShouldClose())
    {
        float elapsed = GetTime();
        
        if (IsKeyPressed(KEY_F2) && (settings.state & MENU_LOCK) == 0)
        {
            settings.state ^= MENU_SHOW;
        }

        settings_controls();

        rlEnableShader(dif_prog);
            rlSetUniform(
                dif_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlSetUniform(
                dif_dec_loc,
                &settings.decay_rate,
                SHADER_UNIFORM_FLOAT,
                1
            );
            rlComputeShaderDispatch(TEX_WIDTH * TEX_HEIGHT / WAVES, 1, 1);
        rlDisableShader();

        rlEnableShader(part_prog);
            rlSetUniform(
                part_res_loc,
                &res,
                SHADER_UNIFORM_IVEC2,
                1
            );

            rlSetUniform(
                part_speed_loc,
                &settings.rotation_speed,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                part_time_loc,
                &elapsed,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                part_size_loc,
                &settings.sensor_size,
                SHADER_UNIFORM_INT,
                1
            );

            float angle = ((float)settings.sense_angle * PI) / 180;
            rlSetUniform(
                part_angle_loc,
                &angle,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                part_dstoff_loc,
                &settings.angle_offset,
                SHADER_UNIFORM_FLOAT,
                1
            );

            rlSetUniform(
                part_thresh_loc,
                &settings.density_threshold,
                SHADER_UNIFORM_INT,
                1
            );
            rlComputeShaderDispatch(BLOB_COUNT / WAVES, 1, 1);
        rlDisableShader();

        BeginDrawing();
            ClearBackground(BLACK);
                DrawTexturePro(
                    blob_map,
                    TEX_RECT,
                    WIN_RECT,
                    Vector2Zero(),
                    0,
                    WHITE
                );
            draw_settings();
        EndDrawing();
        settings.cooldown += 1.0 / GetFPS();
    }

    SetTraceLogLevel(LOG_WARNING);
    
    rlUnloadShaderBuffer(ssbo_pos);
    rlUnloadShaderBuffer(ssbo_rot);

    rlUnloadShaderProgram(part_prog);
    rlUnloadShaderProgram(dif_prog);

    rlUnloadTexture(blob_map.id);

    CloseWindow();

    return 0;
}
