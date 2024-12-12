#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
// Instanced inputs
layout (location = 2) in mat3 in_instance_transforms;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 projection;
uniform float z_pos;

void main()
{
	texcoord = in_texcoord;
	vec3 pos = projection * in_instance_transforms * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, z_pos, 1.0);
}