#version 330
in vec2 in_position;
in vec2 in_texcoord;

out vec2 texcoord;

uniform mat3 projection;
uniform float z_pos;

void main() {
    vec3 pos = projection * vec3(in_position, 1.0);
    gl_Position = vec4(pos.xy, z_pos, 1.0);
    texcoord = in_texcoord;
}
