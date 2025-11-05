#include "renderer/renderer.h"

#define RENDERER_NO_INTERPOLATION false
#define TEXTS_ARENA_SIZE 16000

namespace Render {
	Context* init(Windowing::Context* window, Arena* arena) 
	{
		return platform_render_init(window, arena);
	}

	void update(Context* renderer, State* render_state, Windowing::Context* window, Arena* arena)
	{
		platform_render_update(renderer, render_state, window, arena);
	}

	void advance_state(State* previous, State* current)
	{
		arena_destroy(&previous->texts_arena);
		*previous = *current;
		*current = {};
		arena_init(&current->texts_arena, TEXTS_ARENA_SIZE);
	}

	void character(State* state, char ch, float x, float y, float scale, float r, float g, float b, float a)
	{
		FontGlyph glyph = state->font.font_glyphs[ch];
		Character c;

		c.src[0] = ((float)glyph->x) / state->font.atlas_length;
		c.src[1] = ((float)glyph->y) / state->font.atlas_length;
		c.src[2] = ((float)glyph->w) / state->font.atlas_length;
		c.src[3] = ((float)glyph->h) / state->font.atlas_length;

		c.dst[0] = xpos * scale;
		c.dst[1] = ypos * scale;
		c.dst[2] = w * scale;
		c.dst[3] = h * scale;

		c.color[0] = r;
		c.color[1] = g;
		c.color[2] = b;
		c.color[3] = a;

		state->characters[state->characters_len] = c;
		state->characters_len++;
	}

	void text_line(State* state, const* char string, float x, float y, float r, float g, float b, float a)
	{
		float x = x;
		float y = y;

		i32 len = strlen(string);
		for(i32 i = 0; i < len; i++) {
			FontGlyph glyph = 
			char c = string[i];

			float cur_x = x + c->bearing[0];
			float cur_y = y - (c->h - c->bearing[1]);
			Render::character(state, c, cur_x, cur_y, r, g, b, a);
	        x += (c->advance >> 6);
		}
	}

	State interpolate_states(State* previous, State* current, f32 t)
	{
#if RENDERER_NO_INTERPOLATION
		return *current;
#endif

		// TODO - handle differing numbers of cubes. we assert that this isn't the case for now.
		assert(previous->rects_len == current->rects_len);

		Render::State interpolated = *current;
		interpolated.rects_len = current->rects_len;

		for(u32 i = 0; i < previous->rects_len; i++) {
			interpolated.rects[i].x = lerp(previous->rects[i].x, current->rects[i].x, t);
			interpolated.rects[i].y = lerp(previous->rects[i].y, current->rects[i].y, t);
			interpolated.rects[i].w = lerp(previous->rects[i].w, current->rects[i].w, t);
			interpolated.rects[i].h = lerp(previous->rects[i].h, current->rects[i].h, t);
		}

		return interpolated;
	}
}
