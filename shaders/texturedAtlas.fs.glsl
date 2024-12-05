#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
// Texture position and scaling is a percentage
uniform vec2 tex_pos; // Top left position of texture on atlas
uniform vec2 tex_area; // Size of the texture on atlas

// Output color
layout(location = 0) out vec4 color;

void main() {
    vec2 shifted_texcoords = (texcoord * tex_area) + tex_pos;
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(shifted_texcoords.x, shifted_texcoords.y));
}
