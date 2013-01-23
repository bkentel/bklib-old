#include "pch.hpp"
#include "renderer2d.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <Shlobj.h> //font path

namespace gfx = ::bklib::gfx;
namespace gl  = ::bklib::gl;

struct gfx::font_renderer::impl_t {
public:
    static size_t const TEX_SIZE    = 1024;
    static size_t const CELL_SIZE_X = (TEX_SIZE / CELL_SIZE);
    static size_t const CELL_SIZE_Y = (TEX_SIZE / CELL_SIZE);
    static size_t const CELL_COUNT  = (CELL_SIZE_X * CELL_SIZE_Y);

    impl_t(render_state_2d& state);

    void draw_text(utf8string const& string);
private:
    struct glyph_info {
        FT_UInt          index;
        FT_Glyph_Metrics metrics;
    };

    glyph_info on_fill_cache_(size_t where, utf32codepoint code);

    render_state_2d& state_;
    
    cache<CELL_COUNT, utf32codepoint, glyph_info> glyph_cache_;

    FT_Library library_;
    FT_Face    face_;

    gl::texture_object<gl::texture::target::tex_2d> cache_texture_;

    gl::vertex_array              glyphs_array_;
    gl::buffer_object<glyph_rect> glyphs_;
};

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

gfx::font_renderer::impl_t::impl_t(render_state_2d& state)
    : state_(state)
    , glyph_cache_(
        std::bind(
            &impl_t::on_fill_cache_,
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
    cache_texture_.set_min_filter(gl::texture::min_filter::nearest);
    cache_texture_.set_mag_filter(gl::texture::mag_filter::nearest);

    cache_texture_.create(
        TEX_SIZE, TEX_SIZE,
        gl::texture::internal_format::r8,
        gl::texture::data_format::r,
        gl::texture::data_type::byte_u
    );

    glyphs_.allocate(100);

    glyphs_array_.bind();
        glyphs_.bind();

        glyphs_array_.enable_attribute(state_.attr_pos);
        glyphs_array_.enable_attribute(state_.attr_col);
        glyphs_array_.enable_attribute(state_.attr_tex);

        glyphs_array_.set_attribute_pointer<glyph_rect::vertex::position_t>(state_.attr_pos);
        glyphs_array_.set_attribute_pointer<glyph_rect::vertex::color_t>(state_.attr_col);
        glyphs_array_.set_attribute_pointer<glyph_rect::vertex::tex_coord_t>(state_.attr_tex);
    glyphs_array_.unbind();
}

gfx::font_renderer::impl_t::glyph_info gfx::font_renderer::impl_t::on_fill_cache_(
    size_t                const where,
    bklib::utf32codepoint const code
) {
    uint8_t test_glyph[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

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

    //for (int yy = 0; yy < slot->bitmap.rows; ++yy) {
    //    for (int xx = 0; xx < slot->bitmap.width; ++xx) {
    //        test_glyph[yy * 16 + xx] = 
    //            slot->bitmap.buffer[yy * slot->bitmap.width + xx];
    //    }
    //}

    //cache_texture_.update(
    //    x*CELL_SIZE,
    //    y*CELL_SIZE,
    //    16,
    //    16,
    //    gl::texture::data_format::r,
    //    gl::texture::data_type::byte_u,
    //    test_glyph
    //);

    ::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        

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

void gfx::font_renderer::impl_t::draw_text(bklib::utf8string const& string) {
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

        x += 16 << 6;//info.metrics.horiAdvance;

        FT_UInt const right = info.index;

        if (has_kerning && left) {
            FT_Vector kerning;
            FT_Error const result =
                FT_Get_Kerning(face_, left, right, FT_KERNING_DEFAULT, &kerning);

            if (result) {
                BK_TODO_BREAK;
            }

            //x += kerning.x;
        }

        left = right;
        element++;
    }

    ::glActiveTexture(GL_TEXTURE0);
    state_.base_texture_loc.set(0);
    this->cache_texture_.bind();

    glyphs_array_.bind();
    ::glDrawArrays(GL_TRIANGLES, 0, 6*element);
}

gfx::glyph_rect::glyph_rect(short x, short y, short tx, short ty) {
    static auto const SIZE = gfx::font_renderer::CELL_SIZE;

    auto const pt = y;
    auto const pb = y + SIZE - 0;
    auto const pl = x;
    auto const pr = x + SIZE - 0;

    auto const tt = ty * SIZE;
    auto const tb = ty * SIZE + SIZE - 0;
    auto const tl = tx * SIZE;
    auto const tr = tx * SIZE + SIZE - 0;

    vertices[TR1].position[0]  = pr;
    vertices[TR1].position[1]  = pt;
    vertices[TR1].tex_coord[0] = tr;
    vertices[TR1].tex_coord[1] = tt;

    vertices[TL1].position[0]  = pl;
    vertices[TL1].position[1]  = pt;
    vertices[TL1].tex_coord[0] = tl;
    vertices[TL1].tex_coord[1] = tt;

    vertices[BL1].position[0]  = pl;
    vertices[BL1].position[1]  = pb;
    vertices[BL1].tex_coord[0] = tl;
    vertices[BL1].tex_coord[1] = tb;

    vertices[TR2].position[0]  = pr;
    vertices[TR2].position[1]  = pt;
    vertices[TR2].tex_coord[0] = tr;
    vertices[TR2].tex_coord[1] = tt;

    vertices[BL2].position[0]  = pl;
    vertices[BL2].position[1]  = pb;
    vertices[BL2].tex_coord[0] = tl;
    vertices[BL2].tex_coord[1] = tb;

    vertices[BR2].position[0]  = pr;
    vertices[BR2].position[1]  = pb;
    vertices[BR2].tex_coord[0] = tr;
    vertices[BR2].tex_coord[1] = tb;
}

/////

gfx::font_renderer::~font_renderer() {
}

gfx::font_renderer::font_renderer(render_state_2d& state)
    : impl_(new impl_t(state))
{
}

void gfx::font_renderer::draw_text(bklib::utf8string const& string) {   
    impl_->draw_text(string);
}

///////

gfx::render_state_2d::render_state_2d()
    : vert_shader(gl::shader_type::vertex,   L"../data/shaders/gui.vert")
    , frag_shader(gl::shader_type::fragment, L"../data/shaders/gui.frag")
    , attr_pos(ATTR_POS_LOC)
    , attr_col(ATTR_COL_LOC)
    , attr_tex(ATTR_TEX_LOC)
    , attr_dim(ATTR_DIM_LOC)
{
    vert_shader.compile();
    program.attach(vert_shader);

    frag_shader.compile();
    program.attach(frag_shader);

    program.link();
    
    model_mat = glm::mat4(1.0f);
    view_mat  = glm::mat4(1.0f);
    proj_mat  = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f);
    mvp_mat   = proj_mat * view_mat * model_mat;

    mvp_loc.get_location(            program, "mvp"            );
    render_type_loc.get_location(    program, "render_type"    );
    corner_radius_loc.get_location(  program, "corner_radius"  );
    gradient_steps_loc.get_location( program, "gradient_steps" );
    border_size_loc.get_location(    program, "border_size"    );
    base_texture_loc.get_location(   program, "base_texture"   );
}

gfx::rect_data::color const gfx::rect_data::default_color = {
    0x00, 0x00, 0x00, 0xFF
};

gfx::rect_data::tex_rect const gfx::rect_data::default_tex_coord(
    0, 0, 0, 0
);

gfx::renderer2d::renderer2d()
    : state_()
    , font_renderer_(state_)
    , render_type_(render_state_2d::render_type::round_rect)
    , rects_(250)
{
    state_.program.use();

    state_.mvp_loc.set(&state_.mvp_mat[0][0]);
    state_.render_type_loc.set(static_cast<GLuint>(render_type_));

    rect_array_.bind();
        rects_.buffer().bind();

        rect_array_.enable_attribute(state_.attr_pos);
        rect_array_.enable_attribute(state_.attr_col);
        rect_array_.enable_attribute(state_.attr_tex);
        rect_array_.enable_attribute(state_.attr_dim);

        rect_array_.set_attribute_pointer<rect_data::vertex::position_t>(state_.attr_pos);
        rect_array_.set_attribute_pointer<rect_data::vertex::color_t>(state_.attr_col);
        rect_array_.set_attribute_pointer<rect_data::vertex::tex_coord_t>(state_.attr_tex);
        rect_array_.set_attribute_pointer<rect_data::vertex::dimensions_t>(state_.attr_dim);
    rect_array_.unbind();
}

void gfx::renderer2d::set_viewport(unsigned w, unsigned h) {
    state_.proj_mat = glm::ortho(0.0f, (float)w, (float)h, 0.0f);
    state_.mvp_mat  = state_.proj_mat * state_.view_mat * state_.model_mat;

    state_.mvp_loc.set(&state_.mvp_mat[0][0]);

    ::glViewport(0, 0, w, h);
}

void gfx::renderer2d::update_rect(handle h, rect r) {
    rect_data const data(r);
        
    static auto const offset = rect_data::vertex::position_t::offset;
    static auto const stride = rect_data::vertex::position_t::stride;

    rects_.update(h, data.vertices[0].position, 0 * stride + offset);
    rects_.update(h, data.vertices[1].position, 1 * stride + offset);
    rects_.update(h, data.vertices[2].position, 2 * stride + offset);
    rects_.update(h, data.vertices[3].position, 3 * stride + offset);
}

void gfx::renderer2d::draw_rect(handle rect) const {
    auto const i = rects_.block_index(rect);

    render_type_ = render_state_2d::render_type::round_rect;
    state_.render_type_loc.set(static_cast<GLuint>(render_type_));

    rect_array_.bind();
        
    ::glDrawArrays(
        GL_TRIANGLE_STRIP, i*4, 4
    );
}
