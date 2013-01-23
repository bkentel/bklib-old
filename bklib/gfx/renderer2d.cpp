#include "pch.hpp"

#include "renderer2d.hpp"

namespace gfx = bklib::gfx;

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
    , rects_(250)
    , glyph_rects_(250)
{
    state_.mvp_loc.set(&state_.mvp_mat[0][0]);

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
    proj_mat_ = glm::ortho(0.0f, (float)w, (float)h, 0.0f);
    mvp_mat_  = proj_mat_ * view_mat_ * model_mat_;

    program_.set_uniform(mvp_loc_, mvp_mat_);

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

    rect_array_.bind();
        
    ::glDrawArrays(
        GL_TRIANGLE_STRIP, i*4, 4
    );
}
