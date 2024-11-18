#version 430 core
#include "Position.glsl"

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_index;

layout (binding = 0, std430) readonly buffer wires
{
    uint states[];
};

out vec4 vertex_color;

vec3 make_color(uint red, uint green, uint blue)
{
    return vec3(red, green, blue) / 255.0f;
}

void main()
{
    gl_Position = get_position(in_position);

    uint chunk = states[in_index / 4];
    uint offset = (in_index % 4) * 8;
    uint state = (chunk >> offset) & 0xFFu;

    bool powered = state != 0;
    bool strong = (state & 2u) != 0;

    const vec3 ColorUnpowered = make_color(71, 0, 22);
    const vec3 ColorPowered = make_color(254, 22, 59);
    const vec3 ColorStrong = make_color(247, 137, 27);

    vec3 color = powered ? ColorPowered : ColorUnpowered;
    if (strong) color = ColorStrong;
    vertex_color = vec4(color, 1.0f);
}
