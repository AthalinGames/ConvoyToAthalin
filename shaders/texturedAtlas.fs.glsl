#version 330

// From vertex shader
in vec2 texcoord;
in vec4 tex_offset;

// Application data
uniform sampler2D sampler0;
uniform vec4 fcolor;

// Output color
layout(location = 0) out vec4 color;

void main() {
    vec2 shifted_texcoords = vec2(texcoord.x * tex_offset.z, texcoord.y * tex_offset.w) + vec2(tex_offset.x, tex_offset.y);
    //vec2 shifted_texcoords = vec2(texcoord.x, texcoord.y);
    color = fcolor * texture(sampler0, vec2(shifted_texcoords.x, shifted_texcoords.y));
}
