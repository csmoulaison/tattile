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

void game_update(Game* game, Windowing::Context* window, Render::State* render_state)
{
	game->close_requested = Windowing::button_down(window, game->quit_button);
	game->frames_since_init++;

	// NOW: Need this or it doesn't render the first character. WHY?
	Render::text(render_state, " ", 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

	Render::text(
		render_state, 
		"Faust", 
		32.0f, window->window_height - 96.0f, 
		1.0f, 
		0.8, 0.8f, 0.8f, sin((float)game->frames_since_init * 0.01f));
	Render::text(
		render_state, 
		"After years of wandering the forest, Faust comes upon a crossroads.", 
		32.0f, window->window_height - 164.0f, 
		0.5f, 
		0.8f, 0.8f, 0.8f, 1.0f);
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
