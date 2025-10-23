#include "renderer/renderer.h"

#include "GL/gl3w.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define TEXT_MAX_CHARS 4096

#define MAX_FONT_CHARACTERS 128

struct FontCharacter {
	u32 texture_id;
	u32 size[2];
	i32 bearing[2];
	u32 advance;
};

typedef struct
{
	f32 translation[16];
	f32 scale[16];
} BoxUbo;

typedef struct
{
	// this is a mat2, but must be separated like this for padding requirements.
	alignas(16) f32 transform_a[2];
	alignas(16) f32 transform_b[2];
} TextUbo;

typedef struct {
	FontCharacter font_characters[MAX_FONT_CHARACTERS];

	u32 box_program;
	u32 text_program;

	u32 box_ubo;
	u32 text_ubo;

	u32 quad_vao;
	u32 text_buffer_ssbo;
	u32 font_texture;
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

	gl->box_program = gl_create_program("shaders/box.vert", "shaders/box.frag");
	gl->text_program = gl_create_program("shaders/text.vert", "shaders/text.frag");

	// Vertex arrays/buffers
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

	// Text rendering
	 
	// Font texture loading
	// TODO: Move offline
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) { panic(); }

	FT_Face face;
	if(FT_New_Face(ft, "fonts/BerkeleyMono-Regular.otf", 0, &face)) { panic(); }
	FT_Set_Pixel_Sizes(face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for(unsigned char c = 0; c < MAX_FONT_CHARACTERS; c++) {
		if(FT_Load_Char(face, c, FT_LOAD_RENDER)) { panic(); }

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
			face->glyph->bitmap.buffer
		);
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

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Text SSBO
	//glGenBuffers(1, &gl->text_buffer_ssbo);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TextChar[TEXT_MAX_CHARS]), nullptr, GL_DYNAMIC_DRAW);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gl->text_buffer_ssbo);
	
	// UBOs
	gl->box_ubo = gl_create_ubo(sizeof(BoxUbo), nullptr);
	//gl->text_ubo = gl_create_ubo(sizeof(TextUbo), nullptr);

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
	glUseProgram(gl->box_program);
	u32 box_ubo_block_index = glGetUniformBlockIndex(gl->box_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->box_ubo);
	glUniformBlockBinding(gl->box_program, box_ubo_block_index, 0);

	glBindVertexArray(gl->quad_vao);

	for(u32 i = 0; i < render_state->rects_len; i++)
	{
		// Update ubo
		Rect box = render_state->rects[i];

		BoxUbo box_ubo = {
			.translation = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				box.x, box.y, 0.0f, 1.0f
			},
			.scale = {
				((f32)window->window_height / window->window_width) * box.w, 0.0f, 0.0f, 0.0f,
				0.0f, box.h, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			}
		};

		glBindBuffer(GL_UNIFORM_BUFFER, gl->box_ubo);
		void* p_box_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		memcpy(p_box_ubo, &box_ubo, sizeof(BoxUbo));
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		// Draw
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	/* Update text ubo
	TextUbo text_ubo;	
	f32 text_scale_x = 27.0f / render_state->window_width;
	f32 text_scale_y = 46.0f / render_state->window_height;

	//v2_init(text_ubo.transform_a, text_scale_x,  0);
	//v2_init(text_ubo.transform_b, 0, -text_scale_y);

	glBindBuffer(GL_UNIFORM_BUFFER, gl->text_ubo);
	void* p_text_ubo = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p_text_ubo, &text_ubo, sizeof(text_ubo));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Update text ssbo buffer
	// TODO - 1. Improve text rendering API, moving some of it to game code.
	//        2. Reason about where different calculations should be made between
	//           GPU and host. Probably a lot more here, obviously.
	//        3. Keep in mind any additional features such as text color and things.
	TextChar text_buffer[TEXT_MAX_CHARS] = {0};
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(text_buffer), text_buffer);

	// Draw text
	glUseProgram(gl->text_program);

	u32 text_ubo_block_index = glGetUniformBlockIndex(gl->text_program, "ubo");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, gl->text_ubo);
	glUniformBlockBinding(gl->text_program, text_ubo_block_index, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->font_texture);
	glBindVertexArray(gl->quad_vao); // TODO - redundant, considering we have no other VAOs to bind, yes?
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 0); // TODO - draw correct amount of text quads
	*/
}
