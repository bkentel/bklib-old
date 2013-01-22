#version 330 core

in vec2 fragment_pos;
in vec4 fragment_color;
in vec2 fragment_tex_pos;
in vec4 fragment_dim;
in vec2 fragment_corner_type;

out vec4 color;

uniform sampler2D base_texture;

const int type_round_rect = 0;
const int type_text       = 1;

uniform int render_type = 0;

uniform float corner_radius  = 10.0;
uniform float gradient_steps = 1.0;
uniform float border_size    = 1.0;

void main() {
    if (render_type == type_text) {
        vec4 tex_color = texture(base_texture, fragment_tex_pos);
        color = tex_color;

        return;
    }

    bool round = fragment_corner_type.x >= 0.5;

    vec4 d_side = vec4(
        fragment_dim.z,                  // x = left
        fragment_dim.x - fragment_dim.z, // y = right
        fragment_dim.w,                  // z = top
        fragment_dim.y - fragment_dim.w  // w = bottom
    );

    if (!round && any(lessThanEqual(d_side, vec4(border_size)))) {
        color = vec4(0.8, 0.8, 0.8, 1.0);
        return;
    }

    vec4 d2_side = d_side.xyzw * d_side.xyzw;

    vec4 d2_corner = vec4(
        d2_side.x + d2_side.z, // x = top left
        d2_side.y + d2_side.z, // y = top right
        d2_side.x + d2_side.w, // z = bottom left
        d2_side.y + d2_side.w  // w = botttom right 
    );

    vec4 sum_comp = vec4(
        d_side.x + d_side.z, // x = top left
        d_side.y + d_side.z, // y = top right
        d_side.x + d_side.w, // z = bottom left
        d_side.y + d_side.w  // w = botttom right 
    );

    float r   = corner_radius;
    float r2  = r*r;
    float b   = border_size;
    float rb  = r - b;
    float rb2 = rb*rb;

    vec4 d2_corner_r = d2_corner + 2.0*(r2 - r*sum_comp);

    bvec4 in_corner       = lessThanEqual(d2_corner, vec4(r2));
    bvec4 not_in_corner_r = greaterThan(d2_corner_r, vec4(r2));
    bvec4 not_in_corner_b = greaterThan(d2_corner_r, vec4(rb2));

    if ( round && (
        (in_corner.x && not_in_corner_r.x) ||
        (in_corner.y && not_in_corner_r.y) ||
        (in_corner.z && not_in_corner_r.z) ||
        (in_corner.w && not_in_corner_r.w) )
    ) {
        discard;
    } else if ( round && (
        (in_corner.x && not_in_corner_b.x) ||
        (in_corner.y && not_in_corner_b.y) ||
        (in_corner.z && not_in_corner_b.z) ||
        (in_corner.w && not_in_corner_b.w) ) ||
        any(lessThanEqual(d_side, vec4(b)))
    ) {
        color = vec4(0.8, 0.8, 0.8, 1.0);
    } else {
        color = fragment_color;
    }

    return;
}
