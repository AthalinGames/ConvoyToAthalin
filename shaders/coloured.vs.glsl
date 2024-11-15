#version 330

// !!! Simple shader for colouring basic meshes

// Input attributes
in vec3 in_position;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform float z_pos;

void main()
{
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, z_pos, 1.0);
}
