#version 330 core

in vec4 fragment_color;
in vec2 fragment_pos;
in vec2 fragment_tex;

out vec4 color;

uniform sampler2d texture;

void main() {
    color = fragment_color;
}
