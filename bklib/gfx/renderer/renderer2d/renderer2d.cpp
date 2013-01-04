#include "pch.hpp"
#include "renderer2d.hpp"

#include "platform/win/d2d.ipp"

namespace gfx = ::bklib::gfx2d;

gfx::renderer::renderer(bklib::window& win)
    : impl_(new impl_t(win))
{
}

gfx::renderer::~renderer() {
}

void gfx::renderer::update() {
    impl_->update();
}

void gfx::renderer::draw_begin() {
    impl_->begin();
}

void gfx::renderer::draw_end() {
    impl_->end();
}

void gfx::renderer::clear(gfx::color color) {
    impl_->clear(color);
}

std::unique_ptr<gfx::solid_color_brush>
gfx::renderer::create_solid_brush(color color) {
    return impl_->create_soild_brush(color);
}

void gfx::renderer::fill_rect(rect const& r, brush const& b) {
    impl_->fill_rect(r, b);
}

void gfx::renderer::draw_rect(rect const& r, brush const& b, float width) {
    impl_->draw_rect(r, b, width);
}

gfx::solid_color_brush&
gfx::renderer::get_solid_brush() {
    return impl_->get_solid_brush();
}

void gfx::renderer::resize(unsigned w, unsigned h) {
    impl_->resize(w, h);
}

void gfx::renderer::create_texture(unsigned w, unsigned h, void const* data) {
    impl_->create_texture(w, h, data);
}

void gfx::renderer::draw_text(rect const& r, bklib::utf8string const& text) {
    impl_->draw_text(r, text);
}

void gfx::renderer::draw_texture(rect src, rect dest) {
    impl_->draw_texture(src, dest);
}

void gfx::renderer::translate(float x, float y) {
    impl_->translate(x, y);
}

void gfx::renderer::push_clip_rect(rect const& r) {
    impl_->push_clip_rect(r);
}

void gfx::renderer::pop_clip_rect() {
    impl_->pop_clip_rect();
}

//void gfx::renderer::set_transformation(matrix const& m) {
//    impl_->set_transformation(m);
//}
