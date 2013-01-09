#version 330 core

layout(location = 0) in uvec2 vertex_position;
layout(location = 1) in uvec4 vertex_color;

out vec3 fragment_color;
out vec2 fragment_pos;

uniform mat4 mvp;

uniform uvec2 tile_size;
uniform uvec2 map_size;

uniform usampler2D tile_set;

void main() {
    ivec2 tex_size = textureSize(tile_set);

    int i_tile = vertex_position.x;
    int i_tex  = vertex_position.y;

    uvec2 tile_index(
        i_tile % map_size.x,
        i_tile / map_size.x,
    );

    vec2 tex_index(
        i_tex % tex_size.x,
        i_tex / tex_size.x,
    );

    vec2 tile_pos = tile_index * tile_size;
    vec2 tex_pos  = tex_index  * tex_size;

}
