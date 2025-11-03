#include "renderer/renderer.h"

#include "GL/gl3w.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAX_FONT_CHARS 128
#define MAX_RENDER_CHARS 1024

struct FontCharacter {
	u32 x;
	u32 y;
	u32 w;
	u32 h;
	i32 bearing[2];
	u32 advance;
};

typedef struct
{
	f32 translation[16];
	f32 scale[16];
} BoxUbo;

typedef struct {
	FontCharacter font_characters[MAX_FONT_CHARS];

	u32 quad_program;
	u32 quad_ubo;
	u32 quad_vao;

	u32 text_program;
	u32 text_vao;
	u32 text_vbo;
	
	u32 text_buffer_ssbo;
	u32 font_texture;
	u32 font_texture_length;
} GlBackend;

u32 gl_compile_shader(const char* filename, GLenum type)
{
	// Read file
	FILE* file = fopen(filename, "r");
	if(file == nullptr) 
	{
		panic();
	}
	
	fseek(file, 0, SEEK_END);
	u32 fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char src[fsize];

	char c;
	u32 i = 0;
	while((c = fgetc(file)) != EOF) {
		src[i] = c;
		i++;
	}
	src[i] = '\0';
	fclose(file);

	// Compile shader
	u32 shader = glCreateShader(type);
	const char* src_ptr = src;
	glShaderSource(shader, 1, &src_ptr, 0);
	glCompileShader(shader);

	i32 success;
	char info[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(success == false) {
		glGetShaderInfoLog(shader, 512, nullptr, info);
		printf(info);
		panic();
	}

	return shader;
}

u32 gl_create_program(const char* vert_src, const char* frag_src)
{
	u32 vert_shader = gl_compile_shader(vert_src, GL_VERTEX_SHADER);
	u32 frag_shader = gl_compile_shader(frag_src, GL_FRAGMENT_SHADER);

	u32 program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	return program;
}

u32 gl_create_ubo(u64 size, void* data)
{
	u32 ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ubo;
}

void mat_ortho(float left, float right, float bottom, float top, float near_z, float far_z, float dst[16])
{
	float rl, tb, fn;

	for(u8 i = 0; i < 16; i++) {
		dst[i] = 0.0f;
	}

	rl =  1.0f / (right - left);
	tb =  1.0f / (top   - bottom);
	fn = -1.0f / (far_z - near_z);

	dst[0] = 2.0f * rl;
	dst[5] = 2.0f * tb;
	dst[10] =-fn;
	dst[12] =-(right + left)   * rl;
	dst[13] =-(top   + bottom) * tb;
	dst[14] = near_z * fn;
	dst[15] = 1.0f;
}

Render::Context* platform_render_init(Windowing::Context* window, Arena* arena)
{
	Render::Context* renderer = (Render::Context*)arena_alloc(arena, sizeof(Render::Context));
	renderer->backend = arena_alloc(arena, sizeof(GlBackend));
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(gl3wInit() != 0)
	{
		panic();
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->quad_program = gl_create_program("shaders/quad.vert", "shaders/quad.frag");

	// Quad vertex array/buffer
	f32 quad_vertices[] = {
		 1.0f,  1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,

		 1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f,  1.0f
	};

	glGenVertexArrays(1, &gl->quad_vao);
	glBindVertexArray(gl->quad_vao);

	u32 quad_vbo;
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), 0);

	gl->quad_ubo = gl_create_ubo(sizeof(BoxUbo), nullptr);

	// Text rendering
	gl->text_program = gl_create_program("shaders/text.vert", "shaders/text.frag");

	// Font proprietary loading:
	FILE* font_file = fopen("fonts/out.cmfont", "r");
	if(!font_file) { panic(); }

	u32 atlas_length;
	u32 num_chars;
	fread(&atlas_length, sizeof(u32), 1, font_file);
	fread(&num_chars, sizeof(u32), 1, font_file);
	for(u32 i = 0; i < num_chars; i++) {
		FontCharacter* ch = &gl->font_characters[i];
		fread(&ch->x, sizeof(u32), 1, font_file);
		fread(&ch->y, sizeof(u32), 1, font_file);
		fread(&ch->w, sizeof(u32), 1, font_file);
		fread(&ch->h, sizeof(u32), 1, font_file);

		fread(&ch->bearing[0], sizeof(i32), 1, font_file);
		fread(&ch->bearing[1], sizeof(i32), 1, font_file);

		fread(&ch->advance, sizeof(u32), 1, font_file);
	}
	gl->font_texture_length = atlas_length;
	u32 atlas_area = atlas_length * atlas_length;
	u8 font_pixels[atlas_area];
	fread(font_pixels, sizeof(u8), atlas_area, font_file);
	fclose(font_file);

	glGenTextures(1, &gl->font_texture);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glTexImage2D(
		GL_TEXTURE_2D, 
		0, 
		GL_RED, 
		atlas_length,
		atlas_length,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		font_pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Text buffer ssbo
	glGenBuffers(1, &gl->text_buffer_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float[4]) * 2 * MAX_RENDER_CHARS, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gl->text_buffer_ssbo);

	// Font texture loading
	/* TODO: Move offline
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) { panic(); }

	FT_Face face;
	//if(FT_New_Face(ft, "fonts/BerkeleyMono-Regular.ttf", 0, &face)) { panic(); }
	if(FT_New_Face(ft, "fonts/Ovo-Regular.ttf", 0, &face)) { panic(); }
	FT_Set_Pixel_Sizes(face, 0, 96);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for(unsigned char c = 0; c < MAX_FONT_CHARS; c++) {
		if(FT_Load_Char(face, c, FT_LOAD_RENDER)) { 
			panic(); 
		}

		u32 tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_RED, 
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		FontCharacter font_character = {
			tex,
			{ face->glyph->bitmap.width, face->glyph->bitmap.rows },
			{ face->glyph->bitmap_left, face->glyph->bitmap_top },
			(u32)face->glyph->advance.x
		};
		gl->font_characters[c] = font_character;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Text vertex array/buffer
	glGenVertexArrays(1, &gl->text_vao);
	glBindVertexArray(gl->text_vao);

	glGenBuffers(1, &gl->text_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gl->text_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
	*/

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glViewport(0, 0, window->window_width, window->window_height);
	return renderer;
}

void platform_render_update(Render::Context* renderer, Render::State* render_state, Windowing::Context* window, Arena* arena)
{
	GlBackend* gl = (GlBackend*)renderer->backend;

	if(window->viewport_update_requested)
	{
		glViewport(0, 0, window->window_width, window->window_height);
		window->viewport_update_requested = false;
	}
	
	// Gl render
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw rects
	glUseProgram(gl->quad_program);
	u32 quad_ubo_block_index = glGetUniformBlockIndex(gl->quad_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->quad_ubo);
	glUniformBlockBinding(gl->quad_program, quad_ubo_block_index, 0);

	glBindVertexArray(gl->quad_vao);

	for(u32 i = 0; i < render_state->rects_len; i++)
	{
		// Update ubo
		Rect quad = render_state->rects[i];

		BoxUbo quad_ubo = {
			.translation = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				quad.x, quad.y, 0.0f, 1.0f
			},
			.scale = {
				((f32)window->window_height / window->window_width) * quad.w, 0.0f, 0.0f, 0.0f,
				0.0f, quad.h, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			}
		};

		glBindBuffer(GL_UNIFORM_BUFFER, gl->quad_ubo);
		void* p_quad_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		memcpy(p_quad_ubo, &quad_ubo, sizeof(BoxUbo));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	glUseProgram(gl->text_program);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(gl->quad_vao);

	glUniform2f(glGetUniformLocation(gl->text_program, "screen_size"), window->window_width, window->window_height);

	float render_chars[MAX_RENDER_CHARS][8];
	u32 render_chars_len = 0;
	for(u32 i = 0; i < render_state->texts_len; i++) {
		Render::Text* text = &render_state->texts[i];
		float x = text->position[0];
		float y = text->position[1];
		float scale = text->scale;

		// NOW: Text color included with SSBO.
		glUniform4f(glGetUniformLocation(gl->text_program, "text_color"), text->color[0], text->color[1], text->color[2], text->color[3]);

		for(u32 j = 0; j < text->len; j++) {
			float* render_char = render_chars[render_chars_len];
			render_chars_len++;
			FontCharacter* c = &gl->font_characters[text->string[j]];
			
			float xpos = x + c->bearing[0] * scale;
			float ypos = y - (c->h - c->bearing[1]) * scale;
			float w = c->w * scale;
			float h = c->h * scale;

			render_char[0] = ((float)c->x) / gl->font_texture_length;
			render_char[1] = ((float)c->y) / gl->font_texture_length;
			render_char[2] = ((float)c->w) / gl->font_texture_length;
			render_char[3] = ((float)c->h) / gl->font_texture_length;

			render_char[4] = xpos;
			render_char[5] = ypos;
			render_char[6] = w;
			render_char[7] = h;

	        glBindBuffer(GL_ARRAY_BUFFER, 0);
	        glDrawArrays(GL_TRIANGLES, 0, 6);
	        x += (c->advance >> 6) * scale;
		}
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float[8]) * render_chars_len, render_chars);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, render_chars_len);

	// Unbind stuff
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
