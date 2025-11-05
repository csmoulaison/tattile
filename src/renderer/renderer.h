#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "base/base.h"
#include "window/window.h"

#define MAX_RENDER_RECTS 16
#define MAX_FONT_GLYPHS 128
#define MAX_RENDER_CHARS 1024
#define NUM_FONTS 2

namespace Render {
	struct FontGlyph {
		u32 x;
		u32 y;
		u32 w;
		u32 h;
		i32 bearing[2];
		u32 advance;
	};

	struct Font {
		u32 texture_id;
		u32 texture_width;
		FontGlyph glyphs[MAX_FONT_GLYPHS];
	};

	struct Character {
		float src[4];
		float dst[4];
		float color[4];
	};

	// NOW: This shall be used instead of the Character array in state, so that each
	// can reference the font.
	struct CharacterList {
		u32 font_id;
		u16 characters_len;
		Character characters[MAX_RENDER_CHARS];
	};

	struct State {
		Rect rects[MAX_RENDER_RECTS];
		u8 rects_len;

		Character characters[MAX_RENDER_CHARS];
		u32 characters_len;

		u32 font_id;
	};

	struct Context {
		void* backend;

		bool first_frame;
		State previous_state;
		State current_state;

		Font font; 
	};
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena);
void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena);
u32 platform_create_texture_mono(Render::Context* renderer, u8* pixels, u32 w, u32 h);

#endif
