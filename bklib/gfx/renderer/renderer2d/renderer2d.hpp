#pragma once

#include "util/util.hpp"
#include "gfx/gfx.hpp"
#include "window/window.hpp"
#include "common/math.hpp"

namespace bklib {
namespace gfx2d {

typedef math::rect<float> rect;
typedef gfx::color_f  color;

class brush;
class solid_color_brush;

struct translation {
    float x, y, z;
};

//------------------------------------------------------------------------------
// Renderer for 2D graphics and text
//------------------------------------------------------------------------------
class renderer  {
public:
    renderer(window& win);
    ~renderer();

    void translate(float x, float y, float z);

    void update();

    void resize(unsigned w, unsigned h);

    void create_texture(unsigned w, unsigned h, void const* data = nullptr);
    void draw_texture(rect src, rect dest);

    void draw_begin();
    void draw_end();

    void clear(color color);

    std::unique_ptr<solid_color_brush> create_solid_brush(color color);

    void draw_rect(rect const& r, brush const& b, float width = 1.0f);
    void fill_rect(rect const& r, brush const& b);

    void draw_text(rect const& r, utf8string const& text);

    solid_color_brush& get_solid_brush();

    void translate(float x, float y);

    void push_clip_rect(rect const& r);
    void pop_clip_rect();
public:
    struct impl_t;
private:
    std::unique_ptr<impl_t> const impl_;
};

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Base class for all brush types
//------------------------------------------------------------------------------
class brush {
public:
    virtual ~brush() {}
};

//------------------------------------------------------------------------------
// Simple solid color brush
//------------------------------------------------------------------------------
class solid_color_brush : public brush {
public:
    virtual ~solid_color_brush() {}

    virtual color get_color() const = 0;
    virtual void set_color(color const& c) = 0;
};

} //namespace gfx
} //namespace bklib
