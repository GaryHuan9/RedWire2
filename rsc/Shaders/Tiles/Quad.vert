#version 400 core
#include "Position.glsl"

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_color;

out vec4 vertex_color;

void main()
{
    gl_Position = get_position(in_position);
    vertex_color = unpackUnorm4x8(in_color);
}
