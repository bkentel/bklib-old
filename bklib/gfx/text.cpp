#include "pch.hpp"
#include "text.hpp"

#include "gfx/gl.hpp"

#include <Shlobj.h> //font path

namespace gfx = ::bklib::gfx;
namespace gl  = ::bklib::gl;

//==============================================================================
//! 
//==============================================================================
struct glyph_vertex {
    //--------------------------------------------------------------------------
    // (x, y) position.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_2,
        gl::buffer::type::short_s
    > position_data;
    //--------------------------------------------------------------------------
    // (r, g, b, a) color.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_4,
        gl::buffer::type::byte_u
    > color_data;
    //--------------------------------------------------------------------------
    // (s, t) texture coordinates.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_2,
        gl::buffer::type::short_u
    > tex_coord_data;

    enum class attribute {
        position,
        color,
        tex_coord,
    };

    typedef gl::buffer::data_traits_set<
        position_data
      , color_data
      , tex_coord_data
    > data_set;

    typedef data_set::attribute_traits<0>::type       position_t;
    typedef data_set::attribute_traits<1, true>::type color_t;
    typedef data_set::attribute_traits<2>::type       tex_coord_t;

    static size_t const size = data_set::stride;

    position_t::type  position;
    color_t::type     color;
    tex_coord_t::type tex_coord;
};

struct glyph_rect {
    static size_t const vertex_count = 4;
    typedef glyph_vertex vertex;

    glyph_rect(short x, short y, short tx, short ty) {


    }

    std::array<vertex, vertex_count> vertices;
};

FT_Glyph_Metrics gfx::font_renderer::on_fill_cache_(
    size_t where,
    bklib::utf32codepoint code
) {
    auto const x = where % CELL_SIZE_X;
    auto const y = where / CELL_SIZE_Y;

    FT_GlyphSlot const slot = face_->glyph;
    auto result = FT_Load_Char(face_, code, FT_LOAD_RENDER);
    if (result) {
        BK_TODO_BREAK;
    }

    cache_texture_.update(
        x*CELL_SIZE,
        y*CELL_SIZE,
        slot->bitmap.width,
        slot->bitmap.rows,
        gl::texture::data_format::r,
        gl::texture::data_type::byte_u,
        slot->bitmap.buffer
    );

    return slot->metrics;
}

gfx::font_renderer::font_renderer()
    : glyph_cache_(
        std::bind(
            &font_renderer::on_fill_cache_,
            this,
            std::placeholders::_1,
            std::placeholders::_2
        )
    )
{
    static char const FONT_NAME[] = "meiryo.ttc";

    PWSTR font_path = nullptr;
    if (FAILED(
        ::SHGetKnownFolderPath(FOLDERID_Fonts, 0, 0, &font_path)
    )) {
        BK_TODO_BREAK;
    }

    BK_ON_SCOPE_EXIT({
        ::CoTaskMemFree(font_path);
    });

    bklib::utf8string const font = 
        bklib::utf8_16_converter().to_bytes(font_path) + "/" + FONT_NAME;

    auto result = FT_Init_FreeType(&library_);
    if (result) {
        BK_TODO_BREAK;
    }
        
    result = FT_New_Face(
        library_,
        font.c_str(),
        0,
        &face_
    );
        
    if (result == FT_Err_Unknown_File_Format) {
        BK_TODO_BREAK;
    } else if (result) {
        BK_TODO_BREAK;
    }

    result = FT_Set_Pixel_Sizes(face_, 0, 16);
    if (result) {
        BK_TODO_BREAK;
    }

    /////

    cache_texture_.bind();
    cache_texture_.set_min_filter(gl::texture::min_filter::linear);
    cache_texture_.create(
        TEX_SIZE, TEX_SIZE,
        gl::texture::internal_format::r8ui,
        gl::texture::data_format::r,
        gl::texture::data_type::byte_u
    );
}

void gfx::font_renderer::draw_text(bklib::utf8string const& string) {
    for (auto const code_unit : string) {
        auto const  i    = glyph_cache_.get(code_unit);
        auto const& info = glyph_cache_[i].info;


    }
}
