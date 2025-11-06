#include <math.h>

struct Game {
	bool close_requested;
	u32 frames_since_init;

	Windowing::ButtonHandle quit_button;
};

Game* game_init(Windowing::Context* window, Arena* arena) 
{
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));

	game->close_requested = false;
	game->frames_since_init = 0;

	game->quit_button = Windowing::register_key(window, Windowing::Keycode::Escape);

	return game;
}

void game_update(Game* game, Windowing::Context* window, Render::Context* renderer)
{
	game->close_requested = Windowing::button_down(window, game->quit_button);
	game->frames_since_init++;

	Render::text_line(
		renderer, 
		"Tattile", 
		32.0f, window->window_height - 32.0f, 
		0.0f, 1.0f,
		0.8, 0.8f, 0.8f, sin((float)game->frames_since_init * 0.01f),
		FONT_FACE_LARGE);
	Render::text_line(
		renderer, 
		"Tattile",
		32.0f, window->window_height -164.0f, 
		0.0f, 1.0f,
		0.8f, 0.8f, 0.8f, 1.0f,
		FONT_FACE_SMALL);

	Render::text_line(
		renderer, 
		"Tattile", 
		window->window_width - 32.0f, 32.0f, 
		1.0f, 0.0f,
		0.8, 0.8f, 0.8f, sin((float)game->frames_since_init * 0.01f),
		FONT_FACE_LARGE);
	Render::text_line(
		renderer, 
		"Tattile",
		window->window_width - 32.0f, 164.0f, 
		1.0f, 0.0f,
		0.8f, 0.8f, 0.8f, 1.0f,
		FONT_FACE_SMALL);

	Render::text_line(
		renderer, 
		"Center",
		window->window_width / 2.0f, window->window_height / 2.0f, 
		0.5f, 0.5f,
		0.8f, 0.8f, 0.8f, 1.0f,
		FONT_FACE_SMALL);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
