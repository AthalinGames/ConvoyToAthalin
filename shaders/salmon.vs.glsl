#version 330

// Input attributes
in vec3 in_position;
in vec3 in_color;

out vec3 vcolor;
out vec2 vpos;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform float z_pos;

void main()
{
	vpos = in_position.xy; // local coordinated before transform
	vcolor = in_color;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0); // why not simply *in_position.xyz ?
	gl_Position = vec4(pos.xy, z_pos, 1.0);
}