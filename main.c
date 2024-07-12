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
#define BG_COLOR CLITERAL(Color){30, 30, 30, 255}
#define COLOR_INVERT(c) CLITERAL(Color){255-(c).r, 255-(c).g, 255-(c).b, (c).a}
#define FG_COLOR COLOR_INVERT(BG_COLOR)

#define SCALE 25
#define SCALE_FACTOR 0.5
#define SMOOTHER_SCALE SCALE_FACTOR / 100

#define WIN_WIDTH 1200
#define WIN_HEIGHT 800
#define WIN_BOUNDS CLITERAL(Rectangle){0, 0, WIN_WIDTH, WIN_HEIGHT}

#define ROW_COUNT (WIN_HEIGHT / SCALE)
#define COL_COUNT (WIN_WIDTH / SCALE)

#define CELL_COUNT (ROW_COUNT * COL_COUNT)

#define OCTAVES 4

#define PARTICULE_SIZE 4
#define PARTICULE_COUNT 64

#define CELL_AT(s, x, y) ((s)->cells[(y) * (s)->cols + (x)])

typedef struct Cell {
	Vector2 pos;
	// scaled position
	Vector3 spos;
	float a;
} Cell;

struct simulation {
	int rows;
	int cols;
	Cell *cells;
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

Vector2 Vector2Rot(Vector2 x, Vector2 org, float angle)
{
	Vector2 res = Vector2Subtract(x, org);
	res = Vector2Rotate(res, angle);
	res = Vector2Add(res, org);

	return res;
}

float zoff = 0;

void sim_seedinit(struct simulation *sim)
{
	float yoff = 0;

	for (int y = 0; y < sim->rows; y++)
	{
		float xoff = 0;

		for (int x = 0; x < sim->cols; x++)
		{
			// float noise = perlin2d(x, y, 0.05, 8);
			// float noise = perlin_fbm2d(8, x, y);
			float noise = perlin_fbm3d(OCTAVES, xoff, yoff, zoff);

			Cell *cell = &CELL_AT(sim, x, y);

			cell->pos.x = (x + 1) * SCALE;
			cell->pos.y = y * SCALE + SCALE / 2.0;

			cell->spos.x = xoff;
			cell->spos.y = yoff;
			cell->spos.z = zoff;

			cell->a = noise * TWO_PI;

			xoff += SCALE_FACTOR;
		}

		yoff += SCALE_FACTOR;
		zoff += randf() * SMOOTHER_SCALE;
	}
}

void sim_update(struct simulation *sim)
{
	Vector2 p1;
	Vector2 p2;
	float noise;

	for (int y = 0; y < sim->rows; y++)
	{
		for (int x = 0; x < sim->cols; x++)
		{
			Cell *cell = &CELL_AT(sim, x, y);
			Vector3 *spos = &cell->spos;
			noise = perlin_fbm3d(OCTAVES, spos->x, spos->y, zoff) * TWO_PI;

			p1.x = x * SCALE + SCALE / 2.0;
			p1.y = y * SCALE + SCALE / 2.0;
		
			p2 = Vector2Rot(cell->pos, p1, noise);

		    DrawLineV(p1, p2, FG_COLOR);
		}

		// zoff += 0.004;
		zoff += randf() * SMOOTHER_SCALE;
	}
}

int main(void)
{
	SetTraceLogLevel(LOG_WARNING);
	SetTargetFPS(60);
	InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");
	SetTraceLogLevel(LOG_INFO);
	DisableEventWaiting();

	g_seed = SEED;

	log_infos();

	// Vector2 particules[PARTICULE_COUNT] = {0};
	Cell cells[CELL_COUNT] = {0};

	struct simulation sim = {
		.rows = ROW_COUNT,
		.cols = COL_COUNT,
		// .cells = calloc(CELL_COUNT, sizeof(Cell))
		.cells = cells
	};

	srand(g_seed);
	// sim_seed(&sim);

	size_t frame_count = 0;
	size_t update_frame = 100;

	sim_seedinit(&sim);

	while (!WindowShouldClose())
	{
		if (frame_count % update_frame == 0)
		{

			BeginDrawing();
				ClearBackground(BG_COLOR);
				sim_update(&sim);
			EndDrawing();
			
		}
		
		// DrawText(TextFormat("%d", GetFPS()), 0, 0, 10, RAYWHITE);
		frame_count++;
	}

	// free(sim.cells);
	SetTraceLogLevel(LOG_WARNING);
	CloseWindow();

	return 0;
}
