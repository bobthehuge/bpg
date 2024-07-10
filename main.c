#include <raylib.h>

// #define BG_COLOR RAYWHITE
#define BG_COLOR (CLITERAL(Color){ 30, 30, 30, 255 })
#define COLOR_INVERT(c) CLITERAL(Color){ ~(char)(c).r, ~(char)(c).g, ~(char)(c).b, (c).a }
#define FG_COLOR COLOR_INVERT(BG_COLOR)

int main (void)
{
	InitWindow(600, 400, "Bob's Particle Game");

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BG_COLOR);
		DrawCircle(275, 175, 50.0, FG_COLOR);

		EndDrawing();
	}
	
	return 0;
}
