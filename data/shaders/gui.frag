#version 330 core

in vec4 fragment_color;
in vec2 fragment_pos;
in vec4 fragment_dim;
in vec2 fragment_tex_pos;
in vec2 fragment_corner_type;

out vec4 color;

uniform float corner_radius  = 10.0;
uniform float gradient_steps = 1.0;
uniform float border_size    = 1.0;

void main() {
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

	//vec2 p = fragment_pos;	  // interpolated vertex position [top, left] to [bottom, right]
	//vec2 d = fragment_dim.xy; // constant width and height
	//vec2 o = fragment_dim.zw; // interpolated offset from [0,0] to [w, h]

	//float x0 = o.x;
	//float y0 = o.y;
	//float x1 = (o.x - d.x);
	//float y1 = (o.y - d.y);

	////circles centered on the corner points of a rectangle
	//vec4 in_test = vec4(
	//	x0*x0 + y0*y0,
	//	x1*x1 + y0*y0,
	//	x0*x0 + y1*y1,
	//	x1*x1 + y1*y1
	//);

	////circles centered on a point offset by r pixels inside the rectangle
 //   //float r = corner_radius;
	//vec4 out_test = vec4(
	//	(x0-r)*(x0-r) + (y0-r)*(y0-r),
	//	(x1+r)*(x1+r) + (y0-r)*(y0-r),
	//	(x0-r)*(x0-r) + (y1+r)*(y1+r),
	//	(x1+r)*(x1+r) + (y1+r)*(y1+r)
	//);

 //   //float r2 = r*r;
	//bvec4 in_b  =    lessThan(in_test,  vec4(r2));
	//bvec4 out_b = greaterThan(out_test, vec4(r2));

	//bvec4 c = bvec4(
	//	in_b.x && out_b.x,
	//	in_b.y && out_b.y,
	//	in_b.z && out_b.z,
	//	in_b.w && out_b.w
	//);

	//if (any(c)) {
	//	discard;
	//}

 //   float r2b = (r - border_size)*(r - border_size);
	//bvec4 in_b2  =    lessThanEqual(in_test,  vec4(r2b));
	//bvec4 out_b2 = greaterThanEqual(out_test, vec4(r2b));

	//bvec4 c2 = bvec4(
	//	in_b2.x && out_b2.x,
	//	in_b2.y && out_b2.y,
	//	in_b2.z && out_b2.z,
	//	in_b2.w && out_b2.w
	//);

 //   if (any(c2)) {
 //       color = vec4(0.0, 0.0, 0.0, 1.0);        
 //   } else
 //   //gradient effect
 //   if (gradient_steps > 1)
 //       color = floor(fragment_color * gradient_steps) / gradient_steps;
 //   else {
 //       if (
 //           (o.x <= border_size) ||
 //           (o.y <= border_size) ||
 //           (d.x - o.x <= border_size) ||
 //           (d.y - o.y <= border_size)
 //       ) {
 //           color = vec4(0.0, 0.0, 0.0, 1.0);
 //       } else {
 //           color = fragment_color;
 //       }
 //   }
}
