#version 430 core
layout (location = 0) in vec2 vert;
out vec2 uv;

struct Char {
	vec4 src;
	vec4 dst;
};

layout(std430, binding = 0) buffer txt
{
	Char string[];
} text;

uniform vec2 screen_size;

void main()
{
	// -1.0 to 1.0 -> 0.0 to 1.0
	vec2 uvert = vec2((vert.x + 1.0f) / 2.0f, (vert.y + 1.0f) / 2.0f);
	uvert = vec2(uvert.x, 1.0 - uvert.y);
	vec2 uvert2 = vec2(uvert.x, 1.0 - uvert.y);

	Char ch = text.string[gl_InstanceID];
	vec2 dst_xy = ch.dst.xy / screen_size;
	vec2 dst_zw = ch.dst.zw / screen_size;
	vec2 pos = dst_xy + dst_zw * uvert2.xy;
	pos -= vec2(1.0f, 0.0f);

	gl_Position = vec4(pos, 0.0f, 1.0f);
	uv = ch.src.xy + ch.src.zw * uvert.xy;
}
