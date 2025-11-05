#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "base/base.h"
#include "window/window.h"

#define MAX_RENDER_RECTS 16
#define MAX_FONT_GLYPHS 128
#define MAX_RENDER_CHARS 1024

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
		u32 atlas_length;
		FontGlyph glyph[MAX_FONT_GLYPHS];
	};

	struct Character {
		float src[4];
		float dst[4];
		float color[4];
	};

	struct Context {
		void* backend;

		// array later.
		Font font; 
	};

	struct State {
		Rect rects[MAX_RENDER_RECTS];
		u8 rects_len;

		Character characters[MAX_RENDER_CHARS];
		u16 characters_len;
	};
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena);
void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena);

#endif
