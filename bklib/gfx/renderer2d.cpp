#include "pch.hpp"

#include "renderer2d.hpp"

namespace gfx = bklib::gfx;

gfx::rect_data::color const gfx::rect_data::default_color = {
    0x00, 0x00, 0x00, 0xFF
};

gfx::rect_data::tex_rect const gfx::rect_data::default_tex_coord(
    0, 0, 0, 0
);

gfx::renderer2d::renderer2d()
    : rects_(100)
{
}
