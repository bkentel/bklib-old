#version 330 core

// Input vertex data, different for all executions of this shader.

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec4 vertex_tex;
layout(location = 3) in vec4 vertex_dim;

out vec4 fragment_color;
out vec2 fragment_pos;
out vec4 fragment_dim;
out vec2 fragment_tex_pos;
out vec2 fragment_corner_type;

uniform mat4 mvp;

void main() {
	gl_Position   = mvp * vec4(vertex_position, 0.0, 1.0);
	gl_Position.z = 0.0;

	fragment_color = vertex_color;
	fragment_pos   = gl_Position.xy;
	fragment_dim   = vertex_dim;

    fragment_tex_pos     = vertex_tex.st;
    fragment_corner_type = vertex_tex.pq;
}
