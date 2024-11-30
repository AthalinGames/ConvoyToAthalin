#version 330

in vec2 texcoord;

uniform sampler2D text;
uniform vec3 fcolor;

layout(location = 0) out vec4 color;

void main() {
    vec4 sampled = vec4(1, 1, 1, texture(text, texcoord).r);
    color = vec4(fcolor, 1.0) * sampled;
}
