#version 400 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_color;

uniform vec2 scale;
uniform vec2 origin;
out vec4 vertex_color;

void main()
{
    gl_Position = vec4(in_position * scale + origin, 0.0f, 1.0f);
    vertex_color = unpackUnorm4x8(in_color);
}
