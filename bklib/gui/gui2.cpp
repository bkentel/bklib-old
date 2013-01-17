#include "pch.hpp"
#include "gui2.hpp"

namespace gui = ::bklib::gui;
namespace gfx = ::bklib::gfx;

// state

gui::state::state(renderer& r)
    : renderer_(r)
    , mouse_listener_(nullptr)
    , keyboard_listener_(nullptr)
    , text_listener_(nullptr)
{
}

// root

gui::root::root(renderer& r)
    : state_(r)
{

}

void gui::root::draw() const {
    std::for_each(
        children_.crbegin(),
        children_.crend(),
        [](child const& w) {
            w->draw();
        }
    );
}

void gui::root::add_widget_(child widget) {   
    children_.emplace_back(
        std::move(widget)
    );
}

gui::root::iterator_ gui::root::get_topmost_widget_at_(value_t const x, value_t const y) {
    return std::find_if(
        std::begin(children_),
        std::end(children_),
        [&](child& w) {
            return math::intersects(point(x, y), w->get_rect());            
        }
    );
}

void gui::root::on_mouse_move(value_t const dx, value_t const dy) {
}

void gui::root::on_mouse_move_to(value_t const x, value_t const y) {
    auto last_mouse = state_.get_mouse_history().current();
    mouse_state_record const rec = { x, y, 0, last_mouse.buttons };

    BK_ON_SCOPE_EXIT({
        state_.push_mouse_record_(rec);
    });

    // get the top-widget as a pointer, or nullptr
    auto const get_topmost = [this](value_t const x, value_t const y) {
        auto const where = get_topmost_widget_at_(x, y);
        return where != children_.end() ? where->get() : nullptr;
    };

    mouse_listener* const listener =
        state_.listener<state::mouse_listener_t>();

    if (listener) {
        listener->on_mouse_move_to(x, y);
    }

    mouse_listener* const curr_topmost = get_topmost(x, y);
    mouse_listener* const last_topmost = get_topmost(last_mouse.x, last_mouse.y);

    if (curr_topmost) {
        if (curr_topmost != last_topmost) {
            if (last_topmost) {
                last_topmost->on_mouse_leave();
            }

            curr_topmost->on_mouse_enter();
        }

        curr_topmost->on_mouse_move(x, y);
    }
}

void gui::root::send_to_top_(iterator_ const where) {
    BK_ASSERT(where != children_.end());
    
    for (auto it = where; it != children_.begin(); ) {
        auto cur = it;
        std::swap(*cur, *(--it));
    }
}

void gui::root::on_mouse_down(button_t button) {
    auto const cur_mouse = state_.get_mouse_history().current();
    mouse_state_record rec = { cur_mouse.x, cur_mouse.y, cur_mouse.scroll, cur_mouse.buttons };
    rec.set_button(button);

    BK_ON_SCOPE_EXIT({
        state_.push_mouse_record_(rec);
    });

    mouse_listener* const listener =
        state_.listener<state::mouse_listener_t>();

    if (listener) {
        listener->on_mouse_down(button);
        return;
    }

    auto const curr_topmost =
        get_topmost_widget_at_(cur_mouse.x, cur_mouse.y);

    if (curr_topmost == children_.end()) return;

    auto const widget = curr_topmost->get();

    send_to_top_(curr_topmost);

    widget->on_mouse_down(button);
}

void gui::root::on_mouse_up(button_t button) {
    auto const cur_mouse = state_.get_mouse_history().current();
    mouse_state_record rec = { cur_mouse.x, cur_mouse.y, cur_mouse.scroll, cur_mouse.buttons };
    rec.clear_button(button);

    BK_ON_SCOPE_EXIT({
        state_.push_mouse_record_(rec);
    });

    mouse_listener* const listener =
        state_.listener<state::mouse_listener_t>();

    if (listener) {
        listener->on_mouse_up(button);
        return;
    }

    auto const curr_topmost =
        get_topmost_widget_at_(cur_mouse.x, cur_mouse.y);

    if (curr_topmost != children_.end()) {
        (*curr_topmost)->on_mouse_up(button);
    }
}

void gui::root::on_mouse_scroll(value_t scroll) {

}

// window

gui::rect gui::window::get_non_client_rect(rect const& r) {
    return rect(
        r.left   - BORDER_SIZE,
        r.top    - TITLE_SIZE,
        r.right  + BORDER_SIZE,
        r.bottom + BORDER_SIZE
    );
}

gui::rect gui::window::get_client_rect(rect const& r) {
    return rect(
        r.left   + BORDER_SIZE,
        r.top    + TITLE_SIZE,
        r.right  - BORDER_SIZE,
        r.bottom - BORDER_SIZE
    );
}

gui::rect gui::window::get_title_rect(rect const& r) {
    return rect(
        r.left,
        r.top,
        r.right,
        r.top    + TITLE_SIZE
    );
}

gui::window::window(state& state, rect const& r)
    : state_(state)
    , non_client_rect_(r)
    , client_rect_(get_client_rect(r))
    , title_rect_(get_title_rect(r))
    , window_state_(window_state::none)
{
    static color const c0 = gfx::make_color<uint8_t>(255, 0, 0, 255);
    static color const c1 = gfx::make_color<uint8_t>(0, 255, 0, 255);
    static color const c2 = gfx::make_color<uint8_t>(0, 0, 255, 255);
    static color const c3 = gfx::make_color<uint8_t>(255, 255, 255, 255);

    auto& renderer = state_.get_renderer();

    renderer::rect_info nc_info(non_client_rect_);
    nc_info.set_corner_type(renderer::rect_info::corner_type::round);
    nc_info.set_color<renderer::rect_info::corner::top_left>(c0);
    nc_info.set_color<renderer::rect_info::corner::top_right>(c0);
    nc_info.set_color<renderer::rect_info::corner::bottom_left>(c2);
    nc_info.set_color<renderer::rect_info::corner::bottom_right>(c2);

    non_client_rect_handle_ = renderer.create_rect(nc_info);
    
    renderer::rect_info cr_info(client_rect_);
    cr_info.set_corner_type(renderer::rect_info::corner_type::sharp);
    cr_info.set_color<renderer::rect_info::corner::top_left>(c3);
    cr_info.set_color<renderer::rect_info::corner::top_right>(c3);
    cr_info.set_color<renderer::rect_info::corner::bottom_left>(c3);
    cr_info.set_color<renderer::rect_info::corner::bottom_right>(c3);    

    client_rect_handle_ = renderer.create_rect(cr_info);
}

void gui::window::draw() const {
    auto const& renderer = state_.get_renderer();

    renderer.draw_rect(non_client_rect_handle_);
    renderer.draw_rect(client_rect_handle_);
}

gui::rect const& gui::window::get_rect() const {
    return non_client_rect_;
}

void gui::window::set_rect(rect const& r) {
    non_client_rect_ = r;
    client_rect_     = get_client_rect(r);

    auto& renderer = state_.get_renderer();

    renderer.update_rect(non_client_rect_handle_, non_client_rect_);
    renderer.update_rect(client_rect_handle_, client_rect_);
}

void gui::window::on_mouse_move(value_t dx, value_t dy) {
}

void gui::window::translate(scalar dx, scalar dy) {
    non_client_rect_.translate(dx, dy);
    client_rect_.translate(dx, dy);
    title_rect_.translate(dx, dy);

    auto& renderer = state_.get_renderer();

    renderer.update_rect(non_client_rect_handle_, non_client_rect_);
    renderer.update_rect(client_rect_handle_, client_rect_);
}

void gui::window::resize(
    rect::side_x sx, scalar dx,
    rect::side_y sy, scalar dy
) {
    auto const& mouse = state_.get_mouse_history().current();
    auto&       r     = non_client_rect_;
    
    range constraint(100, 1000);
    
    // restrict resizing to when the mouse is on the proper side of the border
    switch (sx) {
    case rect::side_x::left :
        if      (dx > 0 && mouse.x < r.left) sx = rect::side_x::none;
        else if (dx < 0 && mouse.x > r.left) sx = rect::side_x::none;
        break;
    case rect::side_x::right :
        if      (dx > 0 && mouse.x < r.right) sx = rect::side_x::none;
        else if (dx < 0 && mouse.x > r.right) sx = rect::side_x::none;
        break;
    }

    // restrict resizing to when the mouse is on the proper side of the border
    switch (sy) {
    case rect::side_y::top :
        if      (dy > 0 && mouse.y < r.top) sy = rect::side_y::none;
        else if (dy < 0 && mouse.y > r.top) sy = rect::side_y::none;
        break;
    case rect::side_y::bottom :
        if      (dy > 0 && mouse.y < r.bottom) sy = rect::side_y::none;
        else if (dy < 0 && mouse.y > r.bottom) sy = rect::side_y::none;
        break;
    }

    auto const rx = r.resize_constrained(sx, dx, constraint);
    auto const ry = r.resize_constrained(sy, dy, constraint);

    // No change.
    if (rx == 0 && ry == 0) return;

    // update the other rects and update the renderer data
    auto& renderer = state_.get_renderer();
    
    client_rect_ = get_client_rect(r);
    title_rect_  = get_title_rect(r);

    renderer.update_rect(non_client_rect_handle_, non_client_rect_);
    renderer.update_rect(client_rect_handle_, client_rect_);
}

void gui::window::on_mouse_move_to(value_t x, value_t y) {
    auto const& mouse = state_.get_mouse_history().current();

    auto const dx = x - mouse.x;
    auto const dy = y - mouse.y;
    
    switch (window_state_) {
    case window_state::moving :
        translate(dx, dy);
        break;
    case window_state::sizing :
        resize(sizing_side_.x, dx, sizing_side_.y, dy);
        break;
    }
}

void gui::window::on_mouse_scroll(value_t scroll) {
}

void gui::window::on_mouse_down(button_t button) {
    BK_ASSERT(window_state_ == window_state::none);

    auto const& mouse = state_.get_mouse_history().current();

    auto const in_border = math::intersects_border<rect::value_type>(
        mouse.x, mouse.y, non_client_rect_, BORDER_SIZE
    );

    auto const in_title = math::intersects<rect::value_type>(
        mouse.x, mouse.y, title_rect_
    );

    if (in_border) {
        window_state_ = window_state::sizing;
        state_.capture<state::mouse_listener_t>(this);
        sizing_side_ = in_border;
        return;
    } else if (in_title) {
        window_state_ = window_state::moving;
        state_.capture<state::mouse_listener_t>(this);
        return;
    }


}

void gui::window::on_mouse_up(button_t button) {
    if (window_state_ != window_state::none) {
        state_.release<state::mouse_listener_t>(this);
        window_state_ = window_state::none;
    }
}

void gui::window::on_mouse_click(button_t button) {
}
void gui::window::on_mouse_double_click(button_t button) {
}
void gui::window::on_mouse_enter() {
}
void gui::window::on_mouse_leave() {
}
void gui::window::on_mouse_hover() {
}
void gui::window::on_gain_mouse_capture() {
}
void gui::window::on_lose_mouse_capture() {
}
