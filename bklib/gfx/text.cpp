#include "pch.hpp"
#include "text.hpp"

#include "gfx/gl.hpp"

#include <Shlobj.h> //font path

namespace gfx = ::bklib::gfx;
namespace gl  = ::bklib::gl;


gfx::glyph_rect::glyph_rect(short x, short y, short tx, short ty) {
    static auto const SIZE = gfx::font_renderer::CELL_SIZE;
        
    vertices[TR1].position[0]  = x + SIZE;
    vertices[TR1].position[1]  = y;
    vertices[TR1].tex_coord[0] = (tx + 1) * SIZE;
    vertices[TR1].tex_coord[1] = (ty + 0) * SIZE;

    vertices[TL1].position[0]  = x;
    vertices[TL1].position[1]  = y;
    vertices[TL1].tex_coord[0] = (tx + 0) * SIZE;
    vertices[TL1].tex_coord[1] = (ty + 0) * SIZE;

    vertices[BL1].position[0]  = x;
    vertices[BL1].position[1]  = y + SIZE;
    vertices[BL1].tex_coord[0] = (tx + 0) * SIZE;
    vertices[BL1].tex_coord[1] = (ty + 1) * SIZE;

    vertices[TR2].position[0]  = x + SIZE;
    vertices[TR2].position[1]  = y;
    vertices[TR2].tex_coord[0] = (tx + 1) * SIZE;
    vertices[TR2].tex_coord[1] = (ty + 0) * SIZE;

    vertices[BL2].position[0]  = x;
    vertices[BL2].position[1]  = y + SIZE;
    vertices[BL2].tex_coord[0] = (tx + 0) * SIZE;
    vertices[BL2].tex_coord[1] = (ty + 1) * SIZE;

    vertices[BR2].position[0]  = x + SIZE;
    vertices[BR2].position[1]  = y + SIZE;
    vertices[BL2].tex_coord[0] = (tx + 1) * SIZE;
    vertices[BL2].tex_coord[1] = (ty + 1) * SIZE;
}


gfx::font_renderer::glyph_info gfx::font_renderer::on_fill_cache_(
    size_t                const where,
    bklib::utf32codepoint const code
) {
    auto const x = where % CELL_SIZE_X;
    auto const y = where / CELL_SIZE_X;

    auto const index = FT_Get_Char_Index(face_, code);
    if (index == 0) {
        BK_TODO_BREAK;
    }

    FT_GlyphSlot const slot = face_->glyph;
    auto const result = FT_Load_Glyph(face_, index, FT_LOAD_RENDER);
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

    glyph_info const info = {
        index,
        slot->metrics
    };

    return info;
}

namespace {

bklib::utf8string get_font_path() {
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

    return bklib::utf8_16_converter().to_bytes(font_path) + "/" + FONT_NAME;
}

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
    auto const font = get_font_path();

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

    result = FT_Set_Pixel_Sizes(face_, 0, CELL_SIZE);
    if (result) {
        BK_TODO_BREAK;
    }

    /////

    cache_texture_.bind();
    cache_texture_.set_min_filter(gl::texture::min_filter::linear);
    cache_texture_.create(
        TEX_SIZE, TEX_SIZE,
        gl::texture::internal_format::r8,
        gl::texture::data_format::r,
        gl::texture::data_type::byte_u
    );

    glyphs_.allocate(100);

    gl::id::attribute position(0);
    gl::id::attribute color(1);
    gl::id::attribute tex_coord(2);

    glyphs_array_.bind();

    glyphs_.bind();

    glyphs_array_.enable_attribute(position);
    glyphs_array_.enable_attribute(color);
    glyphs_array_.enable_attribute(tex_coord);

    glyphs_array_.set_attribute_pointer<glyph_rect::vertex::position_t>(position);
    glyphs_array_.set_attribute_pointer<glyph_rect::vertex::color_t>(color);
    glyphs_array_.set_attribute_pointer<glyph_rect::vertex::tex_coord_t>(tex_coord);

    glyphs_array_.unbind();
}

void gfx::font_renderer::draw_text(bklib::utf8string const& string) {
    FT_Pos x = 0;
    FT_Pos y = 0;
    
    FT_UInt left = 0;

    std::vector<glyph_rect> rects;

    size_t element = 0;

    for (auto const code_unit : string) {
        auto const  i    = glyph_cache_.get(code_unit);
        auto const& info = glyph_cache_[i].info;

        short const tx = i % CELL_SIZE_X;
        short const ty = i / CELL_SIZE_X;

        bool const has_kerning = FT_HAS_KERNING(face_);

        glyph_rect const gr = glyph_rect(x >> 6, y >> 6, tx, ty);
        rects.emplace_back(gr);

        glyphs_.update(element, gr);

        x += info.metrics.horiAdvance;

        FT_UInt const right = info.index;

        if (has_kerning && left) {
            FT_Vector kerning;
            FT_Error const result =
                FT_Get_Kerning(face_, left, right, FT_KERNING_DEFAULT, &kerning);

            if (result) {
                BK_TODO_BREAK;
            }

            x += kerning.x;
        }

        left = right;
        element++;
    }

    glyphs_array_.bind();

    ::glDrawArrays(GL_TRIANGLES, 0, 6*element);

    OutputDebugStringA(std::to_string(x).c_str());
}
