#version 430 core
in vec2 uv;
out vec4 frag_color;

uniform sampler2D tex;
uniform vec3 text_color;

void main()
{
	frag_color = vec4(text_color, 1.0) * vec4(1.0, 1.0, 1.0, texture(tex, uv).r);
}
