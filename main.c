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
#define FADE_COLOR CLITERAL(Color){0, 0, 0, 5}

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

#define CELL_AT(s, x, y) ((s)->cells[(y) * (s)->cols + (x)])
// #define DRAW_BLOB(vec2) DrawCircleV((vec2), BLOB_SIZE, FG_COLOR)
#define DRAW_BLOB(vec2) DrawPixelV((vec2), WHITE)

typedef struct Cell {
	Vector2 pos;
	// scaled position
	Vector3 spos;
	float a;
} Cell;

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
	Cell *cells;
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

void sim_init_field(struct simulation *sim)
{
	float yoff = 0;

	for (int y = 0; y < sim->rows; y++)
	{
		float xoff = 0;

		for (int x = 0; x < sim->cols; x++)
		{
			Cell *cell = &CELL_AT(sim, x, y);

			cell->pos.x = (x + 1) * SCALE;
			cell->pos.y = y * SCALE + SCALE / 2.0;

			cell->spos.x = xoff;
			cell->spos.y = yoff;
			cell->spos.z = sim->zoff;

			xoff += SCALE_FACTOR;
		}

		yoff += SCALE_FACTOR;
	}
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

void sim_update_field(struct simulation *sim)
{
	for (int y = 0; y < sim->rows; y++)
	{
		for (int x = 0; x < sim->cols; x++)
		{
			Cell *cell = &CELL_AT(sim, x, y);
			Vector3 *spos = &cell->spos;

			// we normalize because perlin_fbm3d is in [-1; 1]
			cell->a = 
				perlin_fbm3d(OCTAVES, spos->x, spos->y, sim->zoff);
			cell->a *= PI;
			// cell->a *= MAG;
		}

		sim->zoff += SMOOTHER_SCALE;
		// sim->zoff += randf() * SMOOTHER_SCALE;
		// sim->zoff += Lerp(-10, 10, randf() * SMOOTHER_SCALE);
	}
}

void sim_update_blobs(struct simulation *sim)
{
	for (int i = 0; i < BLOB_COUNT; i++)
	{
		Blob *blob = &sim->blobs[i];
		Vector2 *bpos = &blob->pos;
		Vector2 *vel = &blob->vel;

		int x = floorf(bpos->x / SCALE);
		int y = floorf(bpos->y / SCALE);

		Cell *cell = &CELL_AT(sim, x, y);

		// a * MAG -> [0; TWO_PI * MAG]
		// rot -> [0; TWO_PI]
		// float a = fmodf(cell->a * MAG, TWO_PI);
		blob->rot = fmodf(blob->rot + cell->a * MAG, PI);

		if (bpos->x >= WIN_WIDTH - 1 || bpos->x <= 0)
			vel->x *= -1;

		if (bpos->y >= WIN_HEIGHT - 1 || bpos->y <= 0)
			vel->y *= -1;

		float nx = bpos->x + cosf(blob->rot) * vel->x;
		float ny = bpos->y + sinf(blob->rot) * vel->y;

		bpos->x = nx;
		bpos->y = ny;
		// bpos->x = xclamp(nx);
		// bpos->y = yclamp(ny);
	}
}

void sim_draw_field(struct simulation *sim)
{
	Vector2 p1;
	Vector2 p2;

	for (int y = 0; y < sim->rows; y++)
	{
		for (int x = 0; x < sim->cols; x++)
		{
			Cell *cell = &CELL_AT(sim, x, y);

			p1.x = x * SCALE + SCALE / 2.0;
			p1.y = y * SCALE + SCALE / 2.0;

			p2 = Vector2Rot(cell->pos, p1, cell->a);
		    DrawLineV(p1, p2, FG_COLOR);
		}
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
	SetTraceLogLevel(LOG_INFO);

	g_seed = SEED;

	log_infos();

	Blob points[BLOB_COUNT] = {0};
	Cell cells[CELL_COUNT] = {0};

	struct simulation sim = {
		.rows = ROW_COUNT,
		.cols = COL_COUNT,
		.zoff = 0,
		// .cells = calloc(CELL_COUNT, sizeof(Cell))
		.cells = cells,
		.blobs = points
	};

	srand(g_seed);

	size_t frame_count = 0;
	size_t update_field_frame = 1000;
	size_t update_blob_frame = 100;
	size_t draw_frame = 100;

	sim_init_field(&sim);
	sim_init_blobs(&sim);

	ClearBackground(BG_COLOR);

	while (!WindowShouldClose())
	{
		if (!(frame_count % update_field_frame))
		{
			sim_update_field(&sim);
		}

		if (!(frame_count % update_blob_frame))
		{
			sim_update_blobs(&sim);
		}
		
		if (!(frame_count % draw_frame))
		{
			BeginDrawing();
				DrawRectangle(0,0, WIN_WIDTH, WIN_WIDTH, FADE_COLOR);
				// sim_draw_field(&sim);
				sim_draw_blobs(&sim);
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
