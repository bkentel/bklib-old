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

    mvp_loc            = program.get_uniform_location("mvp");
    render_type_loc    = program.get_uniform_location("render_type");
    corner_radius_loc  = program.get_uniform_location("corner_radius");
    gradient_steps_loc = program.get_uniform_location("gradient_steps");
    border_size_loc    = program.get_uniform_location("border_size");
    base_texture_loc   = program.get_uniform_location("base_texture");
}

gfx::rect_data::color const gfx::rect_data::default_color = {
    0x00, 0x00, 0x00, 0xFF
};

gfx::rect_data::tex_rect const gfx::rect_data::default_tex_coord(
    0, 0, 0, 0
);

gfx::renderer2d::renderer2d()
    : vert_shader_(gl::shader_type::vertex,   L"../data/shaders/gui.vert")
    , frag_shader_(gl::shader_type::fragment, L"../data/shaders/gui.frag")
    , rects_(250)
    , glyph_rects_(250)
{
    model_mat_ = glm::mat4(1.0f);
    view_mat_  = glm::mat4(1.0f);
    proj_mat_  = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f);
    mvp_mat_   = proj_mat_ * view_mat_ * model_mat_;
          
    vert_shader_.compile();
    program_.attach(vert_shader_);

    frag_shader_.compile();
    program_.attach(frag_shader_);

    program_.link();

    program_.use();
        
    mvp_loc_ = program_.get_uniform_location("mvp");   
    program_.set_uniform(mvp_loc_, mvp_mat_);

    gl::id::attribute position(0);
    gl::id::attribute color(1);
    gl::id::attribute tex_coord(2);
    gl::id::attribute dimensions(3);

    rect_array_.bind();

    rects_.buffer().bind();

    rect_array_.enable_attribute(position);
    rect_array_.enable_attribute(color);
    rect_array_.enable_attribute(tex_coord);
    rect_array_.enable_attribute(dimensions);

    rect_array_.set_attribute_pointer<rect_data::vertex::position_t>(position);
    rect_array_.set_attribute_pointer<rect_data::vertex::color_t>(color);
    rect_array_.set_attribute_pointer<rect_data::vertex::tex_coord_t>(tex_coord);
    rect_array_.set_attribute_pointer<rect_data::vertex::dimensions_t>(dimensions);

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
