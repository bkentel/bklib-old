#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gfx/gl.hpp"

namespace bklib {
namespace gfx {

class font_renderer {
public:
    ~font_renderer() {

    }

    font_renderer();

    void init_text();
    void draw_text(utf8string const& string);

    void draw_text(platform_string const string) {}
    //void draw_text(utf8string      const string) {}
private:
    FT_Library library_;
    FT_Face    face_;
};

} // namespace gfx
} // namespace bklib
