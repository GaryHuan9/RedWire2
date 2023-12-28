#version 430 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in uint in_index;

layout (binding = 2, std430) readonly buffer wires
{
    uint states[];
};

uniform vec2 scale;
uniform vec2 origin;
out vec4 vertex_color;

void main()
{
    gl_Position = vec4(in_position * scale + origin, 0.0f, 1.0f);

    uint chunk = states[in_index / 4];
    uint offset = (in_index % 4) * 8;
    uint byte = (chunk >> offset) & 0xFFu;
    bool state = byte != 0;
    vertex_color = vec4(state ? vec3(1.0f, 1.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f), 1.0f);
}
