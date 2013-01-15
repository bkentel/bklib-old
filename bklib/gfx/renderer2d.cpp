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
    : vert_shader_(gl::shader_type::vertex,   L"../data/shaders/gui.vert")
    , frag_shader_(gl::shader_type::fragment, L"../data/shaders/gui.frag")
    , rects_(1000)
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
