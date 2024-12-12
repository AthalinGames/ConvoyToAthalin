#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
// Instanced inputs
layout (location = 2) in mat3 in_instance_transforms;
layout (location = 5) in vec4 in_instance_atlas_positions;

// Passed to fragment shader
out vec2 texcoord;
out vec4 tex_offset;

// Application data
uniform mat3 projection;
uniform float z_pos;

void main() {
    texcoord = in_texcoord;
    tex_offset = in_instance_atlas_positions;
    vec3 pos = projection * in_instance_transforms * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, z_pos, 1.0);
}
