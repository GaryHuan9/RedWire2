uniform vec2 scale;
uniform vec2 origin;
uniform int rotation;

vec2 rotate(vec2 position)
{
    switch (rotation)
    {
        case 1: return vec2(-position.y, position.x);
        case 3: return vec2(position.y, -position.x);
        case 2: return -position;
        default : return position;
    }
}

vec4 get_position(vec2 position)
{
    position = rotate(position) * scale + origin;
    return vec4(position, 0.0f, 1.0f);
}
