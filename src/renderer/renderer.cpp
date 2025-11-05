#include "renderer/renderer.h"

#define RENDERER_NO_INTERPOLATION false
#define TEXTS_ARENA_SIZE 16000

namespace Render {
	Context* init(Windowing::Context* window, Arena* arena) 
	{
		// API specific initialization
		Context* context = platform_render_init(window, arena);
		context->first_frame = true;

		// Font loading
		FILE* font_file = fopen("fonts/out.cmfont", "r");
		if(!font_file) { panic(); }

		Font* font = &context->font;
		u32 num_chars;
		fread(&font->texture_width, sizeof(u32), 1, font_file);
		fread(&num_chars, sizeof(u32), 1, font_file);
		for(u32 i = 0; i < num_chars; i++) {
			FontGlyph* glyph = &font->glyphs[i];
			fread(&glyph->x, sizeof(u32), 1, font_file);
			fread(&glyph->y, sizeof(u32), 1, font_file);
			fread(&glyph->w, sizeof(u32), 1, font_file);
			fread(&glyph->h, sizeof(u32), 1, font_file);
			fread(&glyph->bearing[0], sizeof(i32), 1, font_file);
			fread(&glyph->bearing[1], sizeof(i32), 1, font_file);
			fread(&glyph->advance, sizeof(u32), 1, font_file);
		}

		u32 texture_area = font->texture_width * font->texture_width;
		u8 font_pixels[texture_area];
		fread(font_pixels, sizeof(u8), texture_area, font_file);
		fclose(font_file);

		font->texture_id = platform_create_texture_mono(context, font_pixels, font->texture_width, font->texture_width);

		return context;
	}

	void update(Context* renderer, Windowing::Context* window, float t, Arena* arena)
	{
		Render::State* current = &renderer->current_state;
		Render::State* previous = &renderer->previous_state;
		Render::State interpolated = *current;

		if(renderer->first_frame) {
			renderer->first_frame = false;
			goto skip_interpolation;
		}

#if RENDERER_NO_INTERPOLATION
		goto skip_interpolation;
#endif

		// TODO - handle differing numbers of cubes. we assert that this isn't the case for now.
		assert(renderer->previous_state.rects_len == current->rects_len);

		interpolated.rects_len = current->rects_len;

		for(u32 i = 0; i < previous->rects_len; i++) {
			interpolated.rects[i].x = lerp(previous->rects[i].x, current->rects[i].x, t);
			interpolated.rects[i].y = lerp(previous->rects[i].y, current->rects[i].y, t);
			interpolated.rects[i].w = lerp(previous->rects[i].w, current->rects[i].w, t);
			interpolated.rects[i].h = lerp(previous->rects[i].h, current->rects[i].h, t);
		}

skip_interpolation:
		platform_render_update(renderer, &interpolated, window, arena);
	}

	void advance_state(Context* renderer)
	{
		renderer->previous_state = renderer->current_state;
		renderer->current_state = {};
	}

	void character(Context* context, char ch, float x, float y, float r, float g, float b, float a)
	{
		State* state = &context->current_state;
		FontGlyph* glyph = &context->font.glyphs[ch];
		Character* c = &state->characters[state->characters_len];
		state->characters_len++;

		c->src[0] = ((float)glyph->x) / context->font.texture_width;
		c->src[1] = ((float)glyph->y) / context->font.texture_width;
		c->src[2] = ((float)glyph->w) / context->font.texture_width;
		c->src[3] = ((float)glyph->h) / context->font.texture_width;

		c->dst[0] = x;
		c->dst[1] = y;
		c->dst[2] = glyph->w;
		c->dst[3] = glyph->h;

		c->color[0] = r;
		c->color[1] = g;
		c->color[2] = b;
		c->color[3] = a;

		// NOW: This will of course be changed once we index fonts by render array.
		state->font_id = context->font.texture_id;
	}

	void text_line(Context* context, const char* string, float x, float y, float r, float g, float b, float a)
	{
		State* state = &context->current_state;
		i32 len = strlen(string);
		for(i32 i = 0; i < len; i++) {
			char ch = string[i];
			FontGlyph* glyph = &context->font.glyphs[ch];

			float cur_x = x + glyph->bearing[0];
			float cur_y = y - (glyph->h - glyph->bearing[1]);
			Render::character(context, ch, cur_x, cur_y, r, g, b, a);
	        x += (glyph->advance >> 6);
		}
	}
}
