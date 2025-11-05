#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#include "time/time.cpp"
#include "window/window.cpp"
#include "renderer/renderer.cpp"

#include "game/config.cpp"
#include "game/game.cpp"

i32 main(i32 argc, char** argv)
{
	Arena program_arena;
	arena_init(&program_arena, GIGABYTE);

	Windowing::Context* window = Windowing::init_pre_graphics(&program_arena);
	Render::Context* renderer = Render::init(window, &program_arena); 
	Windowing::init_post_graphics(window);

	Game* game;
	game = game_init(window, &program_arena);

	double time = 0.0f;
	double current_time = Time::seconds();
	double time_accumulator = 0.0f;

	while(game_close_requested(game) != true) {
		double new_time = Time::seconds();
		double frame_time = new_time - current_time;
		if(frame_time > 0.25f) {
			frame_time = 0.25f;
		}
		current_time = new_time;
		time_accumulator += frame_time;

		while(time_accumulator >= BASE_FRAME_LENGTH) {
			Windowing::update(window, &program_arena);

			// Swap render states.
			Render::advance_state(renderer);
			game_update(game, window, renderer);

			time_accumulator -= BASE_FRAME_LENGTH;
			time += BASE_FRAME_LENGTH;
		}

		// Render based on render states now.
		Render::update(renderer, window, time_accumulator / BASE_FRAME_LENGTH, &program_arena);
		Windowing::swap_buffers(window);
	}
}
