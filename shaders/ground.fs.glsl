#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float screen_darken_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec4 fade_color(vec4 in_color)
{
    if (screen_darken_factor > 0)
    in_color -= screen_darken_factor * vec4(0.8, 0.8, 0.8, 0);
    return in_color;
}

void main()
{

    vec4 in_color = texture(screen_texture, coord);
    color = fade_color(color);
}