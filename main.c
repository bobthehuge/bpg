#include <raylib.h>
#include <raymath.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <stdlib.h>
#include <time.h>

#define SEED time(NULL)

#define BTH_PERLIN_IMPLEMENTATION
#include "bth_perlin.h"

// #define BG_COLOR RAYWHITE
#define BG_COLOR CLITERAL(Color){30, 30, 30, 255}
#define COLOR_INVERT(c) CLITERAL(Color){255-(c).r, 255-(c).g, 255-(c).b, (c).a}
#define FG_COLOR COLOR_INVERT(BG_COLOR)

#define WIN_WIDTH 600
#define WIN_HEIGHT 420
#define WIN_BOUNDS CLITERAL(Rectangle){0, 0, WIN_WIDTH, WIN_HEIGHT}

#define ROW_COUNT (WIN_HEIGHT / CELL_SIZE)
#define COL_COUNT (WIN_WIDTH / CELL_SIZE)

#define CELL_SIZE 30
#define CELL_COUNT ROW_COUNT * COL_COUNT

#define PERLIN_FALLOUT 0.05
#define PERLIN_OCTAVES 8

#define CELL_AT(s, x, y) (s)->cells[(y) * (s)->rows + (x)]

struct simulation {
	int rows;
	int cols;
	float *cells;
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
	TraceLog(LOG_INFO, "\tCell size:\t%d", CELL_SIZE);
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

void sim_seed(struct simulation *sim)
{
	for (int y = 0; y < sim->rows; y++)
	{
		for (int x = 0; x < sim->cols; x++)
		{
			// CELL_AT(sim, x, y) = randf() * 2.0 * PI;
			CELL_AT(sim, x, y) = perlin2d(
			    x,
			    y,
			    PERLIN_FALLOUT,
			    PERLIN_OCTAVES
			) * 2.0 * PI;
		}
	}
}

void sim_render(struct simulation *sim)
{
	Vector2 p1;
	Vector2 p2;
	
	for (int y = 0; y < sim->rows; y++)
	{
		for (int x = 0; x < sim->cols; x++)
		{
			p1.x = x * CELL_SIZE + CELL_SIZE / 2.0;
			p1.y = y * CELL_SIZE + CELL_SIZE / 2.0;
			
			p2.x = p1.x + CELL_SIZE/2.0;
			p2.y = p1.y;
			
			float angle = CELL_AT(sim, x, y);
			p2 = Vector2Rot(p2, p1, angle);

		    // DrawLine(sx, sy, px, py, FG_COLOR);
		    DrawLineV(p1, p2, FG_COLOR);
		}
	}
}

int main(void)
{
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(WIN_WIDTH, WIN_HEIGHT, "Bob's Particle Game");
	SetTraceLogLevel(LOG_INFO);

	g_seed = SEED;
	
	log_infos();

	float cells[CELL_COUNT];
	struct simulation sim = {
		.rows = ROW_COUNT,
		.cols = COL_COUNT,
		.cells = cells
	};

	srand(g_seed);
	sim_seed(&sim);

	while (!WindowShouldClose())
	{
		BeginDrawing();
			ClearBackground(BG_COLOR);
			GuiGrid(WIN_BOUNDS, NULL, CELL_SIZE, 1, NULL);

			sim_render(&sim);
		EndDrawing();
	}

	SetTraceLogLevel(LOG_WARNING);
	CloseWindow();
	
	return 0;
}
