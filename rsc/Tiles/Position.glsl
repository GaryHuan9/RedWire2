#define R0

uniform vec2 scale;
uniform vec2 origin;

vec4 get_position(vec2 position)
{
    vec2 result = position * scale + origin;
    return vec4(result, 0.0f, 1.0f);
}

#define R1

uniform vec2 scale;
uniform vec2 origin;

vec4 get_position(vec2 position)
{
    position = vec2(-position.y, position.x);
    vec2 result = position * scale + origin;
    return vec4(result, 0.0f, 1.0f);;
}

#define R2

uniform vec2 scale;
uniform vec2 origin;

vec4 get_position(vec2 position)
{
    vec2 result = -position * scale + origin;
    return vec4(result, 0.0f, 1.0f);
}

#define R3

uniform vec2 scale;
uniform vec2 origin;

vec4 get_position(vec2 position)
{
    position = vec2(position.y, -position.x);
    vec2 result = position * scale + origin;
    return vec4(result, 0.0f, 1.0f);
}
