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
}

bool game_close_requested(Game* game)
{
	return game->close_requested;
}
