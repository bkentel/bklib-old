#version 330 core

in vec3 fragment_color;
in vec2 fragment_pos;
in vec4 fragment_dim;

out vec3 color;

uniform float corner_radius  = 10.0;
uniform float gradient_steps = 1.0;

void main() {
	vec2 p = fragment_pos;	  // interpolated vertex position [top, left] to [bottom, right]
	vec2 d = fragment_dim.xy; // constant width and height
	vec2 o = fragment_dim.zw; // interpolated offset from [0,0] to [w, h]

	float x0 = o.x;
	float y0 = o.y;
	float x1 = (o.x - d.x);
	float y1 = (o.y - d.y);

	//circles centered on the corner points of a rectangle
	vec4 in_test = vec4(
		x0*x0 + y0*y0,
		x1*x1 + y0*y0,
		x0*x0 + y1*y1,
		x1*x1 + y1*y1
	);

	//circles centered on a point offset by r pixels inside the rectangle
    float r = corner_radius;
	vec4 out_test = vec4(
		(x0-r)*(x0-r) + (y0-r)*(y0-r),
		(x1+r)*(x1+r) + (y0-r)*(y0-r),
		(x0-r)*(x0-r) + (y1+r)*(y1+r),
		(x1+r)*(x1+r) + (y1+r)*(y1+r)
	);

    float r2 = r*r;
	bvec4 in_b  =    lessThan(in_test,  vec4(r2));
	bvec4 out_b = greaterThan(out_test, vec4(r2));

	bvec4 c = bvec4(
		in_b.x && out_b.x,
		in_b.y && out_b.y,
		in_b.z && out_b.z,
		in_b.w && out_b.w
	);

	if (any(c)) {
		discard;
	}

    //gradient effect
    if (gradient_steps > 1)
        color = floor(fragment_color * gradient_steps) / gradient_steps;
    else
        color = fragment_color;
}
