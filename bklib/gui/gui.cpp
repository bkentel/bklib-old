#include "pch.hpp"
#include "gui.hpp"

#include "gfx/renderer2d.hpp"

namespace gui = ::bklib::gui;
namespace gfx = ::bklib::gfx;

gui::color_t const gui::default_colors::window    = gfx::make_color<uint8_t>(80, 80, 80, 255);
gui::color_t const gui::default_colors::border    = gfx::make_color<uint8_t>(127, 127, 127, 200);
gui::color_t const gui::default_colors::text      = gfx::make_color<uint8_t>(200, 200, 200, 255);
gui::color_t const gui::default_colors::highlight = gfx::make_color<uint8_t>(180, 180, 180, 255);

////////////////////////////////////////////////////////////////////////////////
// gui_state
////////////////////////////////////////////////////////////////////////////////
gui::gui_state::gui_state(shared_manager manager)
    : input_focus_listener_(nullptr)
    , mouse_input_listener_(nullptr)
    , mouse_state_()
    , mouse_history_(MOUSE_HISTORY_SIZE, mouse_state())
    , ime_manager_(manager)
    , on_redraw_()
{
    manager->listen<bklib::input::ime::manager::event_on_composition_begin>(
    [&] {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_begin_composition();
        }
    });

    manager->listen<bklib::input::ime::manager::event_on_composition_end>(
    [&] {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_end_composition();
        }
    });

    manager->listen<bklib::input::ime::manager::event_on_composition_update>(
    [&](bklib::input::ime::composition::range_list ranges) {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_update_composition(
                std::move(ranges)
            );
        }
    });
}

void
gui::gui_state::capture_mouse_input(pointer p) {
    mouse_input_listener_ = p;
}

void
gui::gui_state::release_mouse_input(pointer p) {
    if (mouse_input_listener_ == p) {
        mouse_input_listener_ = nullptr;
    }
}

void
gui::gui_state::capture_input_focus(pointer p) {
    BK_ASSERT_MSG(p != nullptr, "Null pointer.");
    
    // p already has focus
    if (input_focus_listener_ == p) {
        return;
    }
    
    if (input_focus_listener_) {
        input_focus_listener_->on_focus_lost();
    }

    input_focus_listener_ = p;
    
    ime_cancel_composition();

    auto const result = input_focus_listener_->on_focus_gained();
    if (result.test<widget_base_t::focus_flags::want_text_input>()) {
        ime_manager_->capture_input(true);
    } else {
        ime_manager_->capture_input(false);
    }
}

void
gui::gui_state::release_input_focus(pointer p) {
    BK_ASSERT_MSG(p != nullptr, "Null pointer.");
    if (input_focus_listener_ != p) {
        return; // can't release it if you don't have it
    }

    input_focus_listener_->on_focus_lost();
    input_focus_listener_ = nullptr;

    ime_cancel_composition(); //cancel any composition that may be active
}

void
gui::gui_state::on_mouse_move_to_(signed x, signed y) {
    mouse_history_.push_back(mouse_state_);
    mouse_state_.set(x, y);
}

void
gui::gui_state::on_mouse_button_state_(unsigned button, mouse_state::button_state state) {
    mouse_history_.push_back(mouse_state_);
    mouse_state_.buttons[button] = state;
}

void
gui::gui_state::set_ime_manager_(std::shared_ptr<bklib::input::ime::manager> manager) {
    ime_manager_ = manager;

    manager->listen<bklib::input::ime::manager::event_on_composition_begin>([&] {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_begin_composition();
        }
    });

    manager->listen<bklib::input::ime::manager::event_on_composition_end>([&] {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_end_composition();
        }
    });

    manager->listen<bklib::input::ime::manager::event_on_composition_update>(
    [&](bklib::input::ime::composition::range_list ranges) {
        if (input_focus_listener_) {
            input_focus_listener_->on_input_update_composition(std::move(ranges));
        }
    });
}

void
gui::gui_state::ime_cancel_composition() {
    ime_manager_->cancel_composition();
}

void
gui::gui_state::ime_set_text(bklib::utf8string const& string) {
    ime_manager_->set_text(string);
}
////////////////////////////////////////////////////////////////////////////////
// parent_base_t
////////////////////////////////////////////////////////////////////////////////
gui::parent_base_t::parent_base_t(
    size_t //reserve
) {
}

gui::parent_base_t::handle_t gui::parent_base_t::add_child(unique_t child) {
    auto result = children_.add(std::move(child));
    if (callback_on_child_add_) callback_on_child_add_(*this, *child);
    return result;
}

gui::parent_base_t::unique_t
gui::parent_base_t::remove_child(handle_t handle) {
    auto result = children_.remove(handle);
    if (callback_on_child_remove_) callback_on_child_remove_(*this, *result);
    return result;
}

gui::parent_base_t::child_t& gui::parent_base_t::get_child(handle_t handle) {
    return children_.get(handle);
}

gui::parent_base_t::child_t const&
gui::parent_base_t::get_child(handle_t handle) const {
    return children_.get(handle);
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::parent_base_t, on_child_add) {
    callback_on_child_add_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::parent_base_t, on_child_remove) {
    callback_on_child_remove_ = handler;
}

////////////////////////////////////////////////////////////////////////////////
// widget_base_t
////////////////////////////////////////////////////////////////////////////////
gui::widget_base_t::widget_base_t(rect_t r)
    : gui_state_(nullptr)
    , bounding_rect_(r)
{
}

gui::widget_base_t::~widget_base_t() {
    if (gui_state_) {
        gui_state_->release_input_focus(this);
        gui_state_->release_mouse_input(this);
    }
}

void gui::widget_base_t::set_bounding_rect(rect_t r) {
    bool allow = true;

    if (this->callback_on_resize_) {
        allow = callback_on_resize_(*this, r, bounding_rect_);
    }

    bounding_rect_ = r;
}

void gui::widget_base_t::resize(
    scalar_t dw, scalar_t dh,
    side_x sx, side_y sy
) {
    bounding_rect_.resize(sx, dw);
    bounding_rect_.resize(sy, dh);
}

gui::rect_t gui::widget_base_t::get_bounding_rect() const {
    return bounding_rect_;
}

void gui::widget_base_t::translate(scalar_t dx, scalar_t dy) {
    move_to(
        bounding_rect_.left + dx,
        bounding_rect_.top  + dy
    );
}

void gui::widget_base_t::move_to(scalar_t x, scalar_t y) {
    bool allow = true;

    if (callback_on_move_) {
        auto const new_rect = bounding_rect_;
        bounding_rect_.move_to(x, y);

        allow = callback_on_move_(*this, new_rect, bounding_rect_);
    }

    if (allow) {
        bounding_rect_.move_to(x, y);
    }
}

bool gui::widget_base_t::hit_test(scalar_t x, scalar_t y) const {
    return hit_test(point_t(x, y));
}

bool gui::widget_base_t::hit_test(point_t p) const {
    return math::intersects(p, get_bounding_rect());
}

void gui::widget_base_t::init_draw(renderer_t& renderer) {
}

void gui::widget_base_t::draw(
    renderer_t& //renderer
) const {
}

void gui::widget_base_t::on_mouse_enter() {
    if (callback_on_mouse_enter_) {
        callback_on_mouse_enter_(*this);
    }
}

void gui::widget_base_t::on_mouse_leave() {
    if (callback_on_mouse_leave_) {
        callback_on_mouse_leave_(*this);
    }
}

void gui::widget_base_t::on_mouse_down(unsigned button) {
    if (callback_on_mouse_down_) {
        callback_on_mouse_down_(*this, button);
    }
}

void gui::widget_base_t::on_mouse_up(unsigned button) {
    if (callback_on_mouse_up_) {
        callback_on_mouse_up_(*this, button);
    }

    on_mouse_click(button); //TODO
}

void gui::widget_base_t::on_mouse_click(unsigned button) {
    if (callback_on_mouse_click_) {
        callback_on_mouse_click_(*this, button);
    }
}

void gui::widget_base_t::on_mouse_move(
    unsigned  x, unsigned y,
    signed   dx, signed   dy
) {
    if (callback_on_mouse_move_) {
        callback_on_mouse_move_(*this, x, y, dx, dy);
    }
}

void gui::widget_base_t::on_input_char(bklib::utf32codepoint codepoint) {
    if (callback_on_input_char_) {
        callback_on_input_char_(*this, codepoint);
    }
}

void gui::widget_base_t::set_gui_state(gui_state& state) {
    BK_ASSERT_MSG(gui_state_ == nullptr, "State already set.");
    gui_state_ = std::addressof(state);
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_enter) {
    callback_on_mouse_enter_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_leave) {
    callback_on_mouse_leave_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_down) {
    callback_on_mouse_down_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_up) {
    callback_on_mouse_up_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_click) {
    callback_on_mouse_click_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_mouse_move) {
    callback_on_mouse_move_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_resize) {
    callback_on_resize_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_move) {
    callback_on_move_ = handler;
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::widget_base_t, on_input_char) {
    callback_on_input_char_ = handler;
}

////////////////////////////////////////////////////////////////////////////////
//root
////////////////////////////////////////////////////////////////////////////////
gui::root::root(shared_manager manager)
    : zorder_()
    , gui_state_(manager)
    , ime_candidate_list_()
{
    namespace ime = bklib::input::ime;

    //gui_state_.set_ime_manager_(manager);

    manager->listen<ime::manager::event_on_input_language_change>(
    [&](utf8string&& lang) {
        OutputDebugStringA("gui: Input language changed to: ");
        OutputDebugStringA(lang.c_str());
        OutputDebugStringA("\n");
    });
    
    manager->listen<ime::manager::event_on_input_activate>(
    [&](bool active) {
        if (active) {
            OutputDebugStringA("gui: Ime activated");
        } else {
            OutputDebugStringA("gui: Ime deactivated");
        }
    });

    manager->listen<ime::manager::event_on_candidate_list_begin>(
    [&]() {
        ime_candidate_list_.show(true);
        gui_state_.redraw();
    });

    manager->listen<ime::manager::event_on_candidate_list_end>(
    [&]() {
        ime_candidate_list_.show(false);
        gui_state_.redraw();
    });

    manager->listen<ime::manager::event_on_candidate_list_change_page>(
    [&](unsigned page) {
        ime_candidate_list_.set_current_page(page);
    });

    manager->listen<ime::manager::event_on_candidate_list_change_selection>(
    [&](unsigned selection) {
        ime_candidate_list_.set_current_selection(selection);
        gui_state_.redraw();
    });

    manager->listen<ime::manager::event_on_candidate_list_change_strings>(
    [&](std::vector<utf8string>&& strings) {
        ime_candidate_list_.set_strings(std::move(strings));
    });
}

//------------------------------------------------------------------------------
//! Draw all children in bottom-up z-order.
//------------------------------------------------------------------------------
void gui::root::draw(renderer_t& renderer) const {
    //for (auto const w : reverse_adapter(zorder_)) {
    //    w->draw(renderer);
    //}

    //// The IME candidate list should be drawn top-most.
    //ime_candidate_list_.draw(renderer);
}

gui::root::handle_t gui::root::add_child(unique_t child) {
    zorder_.emplace_front(child.get());
    
    child->set_gui_state(gui_state_);
    child->init_draw(renderer_);
    
    return parent_base_t::add_child(std::move(child));
}

gui::root::unique_t gui::root::remove_child(handle_t handle) {
    auto result = parent_base_t::remove_child(handle);

    zorder_.remove(result.get());

    return result;
}

void gui::root::on_mouse_move(
      int //dx
    , int //dy
) {
}

BK_UTIL_CALLBACK_DEFINE_IMPL(gui::root, on_update) {
    gui_state_.on_redraw_ = handler;
}

//------------------------------------------------------------------------------
//! Called when the relative (to the system window) mouse position has changed.
//------------------------------------------------------------------------------
void gui::root::on_mouse_move_to(int x, int y) {
    // Make sure to update the x,y position.
    BK_ON_SCOPE_EXIT({
        gui_state_.on_mouse_move_to_(x, y);
    });
    
    // If there is a mouse listener, send it the message.
    if (gui_state_.mouse_input_listener_) {
        gui_state_.mouse_input_listener_->on_mouse_move(x, y, 0, 0);
        return;
    }

    // Find the topmost widgets for that are hit by the current and previous
    // mouse position.
    widget_base_t* current = nullptr; //topmost widget the mouse is now over
    widget_base_t* last    = nullptr; //topmost widget the mouse was last over

    auto const     mx = static_cast<scalar_t>(x);
    auto const     my = static_cast<scalar_t>(y);
    auto const last_x = static_cast<scalar_t>(gui_state_.mouse_x());
    auto const last_y = static_cast<scalar_t>(gui_state_.mouse_y());

    for (auto w : zorder_) {
        current = (!current && w->hit_test(mx, my))         ? w : current;
        last    = (!last    && w->hit_test(last_x, last_y)) ? w : last;

        if (current && last) break;
    }

    // If the widget below the mouse has changed, class mouse_enter and
    // mouse_leave.
    if (current != last) {
        if (current) current->on_mouse_enter();
        if (last)    last->on_mouse_leave();
    // Otherwise, call mouse_move as long as it isn't the listener.
    } else if (current) {
        current->on_mouse_move(x, y, 0, 0);
    }
}

//------------------------------------------------------------------------------
//! Called when a mouse button has gone down.
//------------------------------------------------------------------------------
void gui::root::on_mouse_down(unsigned button) {
    // Make sure to update the button state.
    BK_ON_SCOPE_EXIT({
        gui_state_.on_mouse_button_state_(button, mouse_state::button_state::down);    
    });

    // if a widget is listening to mouse events, forward the message to it only.
    if (gui_state_.mouse_input_listener_) {
        gui_state_.mouse_input_listener_->on_mouse_down(button);
        return;
    }

    auto const x = static_cast<scalar_t>(gui_state_.mouse_x());
    auto const y = static_cast<scalar_t>(gui_state_.mouse_y());

    auto it = std::find_if(
        std::begin(zorder_), std::end(zorder_),
        [&](zorder_t::value_type const w) {
            return w->hit_test(x, y);
        }
    );

    // Nothing under the mouse.
    if (it == zorder_.end()) {
        return;
    }

    // Move the widget under the cursor to the top of the zorder if it isn't
    // already.
    if (it != zorder_.begin()) {
        zorder_.push_front(*it);
        zorder_.erase(it);
    }

    gui_state_.capture_input_focus(zorder_.front());
    zorder_.front()->on_mouse_down(button);
}

//------------------------------------------------------------------------------
//! Called when a mouse button has gone up.
//------------------------------------------------------------------------------
void gui::root::on_mouse_up(unsigned button) {
    // Make sure to update the button state.
    BK_ON_SCOPE_EXIT({
        gui_state_.on_mouse_button_state_(button, mouse_state::button_state::up);
    });

    // if a widget is listening to mouse events, forward the message to it only.
    if (gui_state_.mouse_input_listener_) {
        gui_state_.mouse_input_listener_->on_mouse_up(button);
        return;
    }

    auto const x = static_cast<scalar_t>(gui_state_.mouse_x());
    auto const y = static_cast<scalar_t>(gui_state_.mouse_y());

    auto const it = std::find_if(
        std::begin(zorder_), std::end(zorder_),
        [&](zorder_t::value_type const w) {
            return w->hit_test(x, y);
        }
    );

    // Nothing under the mouse.
    if (it == zorder_.end()) {
        return;
    }

    (*it)->on_mouse_up(button);
}

void gui::root::on_key_up(
    key_code_t //key
) {
}

void gui::root::on_key_down(
    key_code_t //key
) {
}

void gui::root::on_input_char(bklib::utf32codepoint codepoint) {
    if (gui_state_.input_focus_listener_) {
        gui_state_.input_focus_listener_->on_input_char(codepoint);
    }
}

//void gui::root::set_ime_manager(shared_manager manager) {
//    namespace ime = bklib::input::ime;
//
//    gui_state_.set_ime_manager_(manager);
//
//    manager->listen<ime::manager::event_on_input_language_change>([&](utf8string&& lang) {
//        OutputDebugStringA("gui: Input language changed to: ");
//        OutputDebugStringA(lang.c_str());
//        OutputDebugStringA("\n");
//    });
//    
//    manager->listen<ime::manager::event_on_input_activate>([&](bool active) {
//        if (active) {
//            OutputDebugStringA("gui: Ime activated");
//        } else {
//            OutputDebugStringA("gui: Ime deactivated");
//        }
//    });
//
//    manager->listen<ime::manager::event_on_candidate_list_begin>([&]() {
//        ime_candidate_list_.show(true);
//        gui_state_.redraw();
//    });
//
//    manager->listen<ime::manager::event_on_candidate_list_end>([&]() {
//        ime_candidate_list_.show(false);
//        gui_state_.redraw();
//    });
//
//    manager->listen<ime::manager::event_on_candidate_list_change_page>([&](unsigned page) {
//        ime_candidate_list_.set_current_page(page);
//    });
//
//    manager->listen<ime::manager::event_on_candidate_list_change_selection>([&](unsigned selection) {
//        ime_candidate_list_.set_current_selection(selection);
//        gui_state_.redraw();
//    });
//
//    manager->listen<ime::manager::event_on_candidate_list_change_strings>([&](std::vector<utf8string>&& strings) {
//        ime_candidate_list_.set_strings(std::move(strings));
//    });
//}

////////////////////////////////////////////////////////////////////////////////
// window
////////////////////////////////////////////////////////////////////////////////
gui::window::window(rect_t r)
    : widget_base_t(r)
    , client_rect_(compute_client_rect_())
    , state_(state::none)
    , back_color_(default_colors::window)
    , border_color_(default_colors::border)
    , title_color_(default_colors::border)
    , text_color_(default_colors::text)
    , width_constraint_(150, 400)
    , height_constraint_(100, 300)
{
}

void gui::window::set_gui_state(gui_state& state) {
    widget_base_t::set_gui_state(state);

    children_.for_each([&](widget_base_t& w) -> bool {
        w.set_gui_state(state);
        return true;
    });
}

gui::rect_t const& gui::window::get_client_rect() const {
    return client_rect_;
}

void gui::window::init_draw(renderer_t& renderer) {
    rect_handle_ = renderer.create_rect(bounding_rect_);
}

void gui::window::draw(renderer_t& renderer) const {
    renderer.draw_rect(rect_handle_);

    //auto& brush = renderer.get_solid_brush();

    //auto const& border = bounding_rect_;
    //auto const& window = get_client_rect();

    //// Draw the non-client area.
    //brush.set_color(title_color_);
    //renderer.fill_rect(border, brush);

    //brush.set_color(border_color_);
    //renderer.draw_rect(border, brush);

    //brush.set_color(text_color_);
    //renderer.draw_text(
    //    compute_header_rect_(),
    //    "Window Title"
    //);

    //// Draw the window client area.
    //brush.set_color(back_color_);
    //renderer.fill_rect(window, brush);

    //brush.set_color(border_color_);
    //renderer.draw_rect(window, brush);

    //// Draw child widgets.
    //auto const ox = math::x(window);
    //auto const oy = math::y(window);

    //// Clip all drawing to the client area.
    //renderer.push_clip_rect(window);
    //    // Translate the coordinate space.
    //    renderer.translate(ox, oy);
    //        children_.for_each_reverse([&](widget_base_t& child) {
    //            child.draw(renderer);
    //        });
    //    renderer.translate(-ox, -oy);
    //renderer.pop_clip_rect();
}

void gui::window::on_mouse_enter() {
    widget_base_t::on_mouse_enter();

    back_color_ = default_colors::highlight;
    gui_state_->redraw();
}

void gui::window::on_mouse_leave() {
    widget_base_t::on_mouse_leave();

    back_color_ = default_colors::window;
    gui_state_->redraw();
}

void gui::window::on_mouse_down(unsigned button) {
    BK_ASSERT_MSG(gui_state_ != nullptr, "GUI state not set.");

    auto const mx = static_cast<scalar_t>(gui_state_->mouse_x());
    auto const my = static_cast<scalar_t>(gui_state_->mouse_y());

    bool const is_in_header = math::intersects(mx, my, compute_header_rect_());
    auto const intersect = math::intersects_border(
        mx, my, bounding_rect_, static_cast<scalar_t>(BORDER_SIZE)
    );

    if (intersect) {
        state_ = state::sizing;
        sizing_info_ = sizing_info(intersect.x, intersect.y);
        gui_state_->capture_mouse_input(this);
    } else if (is_in_header) {
        state_ = state::moving;
        gui_state_->capture_mouse_input(this);
    }

    // translate into this window's coordinate space
    auto const x = mx - client_rect_.left;
    auto const y = my - client_rect_.top;

    children_.for_each([&](widget_base_t& w) -> bool {
        if (w.hit_test(x, y)) {
            w.on_mouse_down(button);
            return false;
        }

        return true;
    });

    widget_base_t::on_mouse_down(button);
    gui_state_->redraw();
}

void gui::window::on_mouse_up(unsigned button) {
    gui_state_->release_mouse_input(this);
    state_ = state::none;

    widget_base_t::on_mouse_up(button);

    back_color_ = default_colors::window;
    gui_state_->redraw();
}

void gui::window::on_mouse_move(
      unsigned x
    , unsigned y
    , signed   //dx
    , signed   //dy
) {
    BK_ASSERT_MSG(gui_state_ != nullptr, "GUI state not set.");

    auto const old_x   = gui_state_->mouse_x();
    auto const old_y   = gui_state_->mouse_y();
    auto const delta_x = static_cast<signed>(x - old_x);
    auto const delta_y = static_cast<signed>(y - old_y);

    if (state_ == state::moving) {
        translate(
            static_cast<scalar_t>(delta_x),
            static_cast<scalar_t>(delta_y)
        );
    } else if (state_ == state::sizing) {
        auto&       r  = bounding_rect_;
        auto const& si = sizing_info_;

        if (si.x != side_x::none) {
            bool allow_x = false;

            if (delta_x > 0) {
                if      ((si.x == side_x::left ) && (x >= r.left) ) allow_x = true;
                else if ((si.x == side_x::right) && (x >= r.right)) allow_x = true;
            } else if (delta_x < 0) {
                if      ((si.x == side_x::left ) && (x <= r.left) ) allow_x = true;
                else if ((si.x == side_x::right) && (x <= r.right)) allow_x = true;
            }

            if (allow_x) r.resize_constrained(
                si.x, static_cast<scalar_t>(delta_x), width_constraint_
            );
        }

        if (si.y != side_y::none) {
            bool allow_y = false;

            if (delta_y > 0) {
                if      ((si.y == side_y::top )   && (y >= r.top)   ) allow_y = true;
                else if ((si.y == side_y::bottom) && (y >= r.bottom)) allow_y = true;
            } else if (delta_y < 0) {
                if      ((si.y == side_y::top )   && (y <= r.top)   ) allow_y = true;
                else if ((si.y == side_y::bottom) && (y <= r.bottom)) allow_y = true;
            }

            if (allow_y) r.resize_constrained(
                si.y, static_cast<scalar_t>(delta_y), height_constraint_
            );
        }
    
        client_rect_ = compute_client_rect_();
        gui_state_->redraw();        
    }
}

void gui::window::resize(scalar_t dw, scalar_t dh, side_x sx, side_y sy) {
    auto& r = bounding_rect_;

    auto const delta_w = (sx == side_x::none) ? 0 :
        r.resize_constrained(sizing_info_.x, dw, width_constraint_);
    auto const delta_h = (sy == side_y::none) ? 0 :
        r.resize_constrained(sizing_info_.y, dh, height_constraint_);

    BK_UNUSED_VAR(delta_w);
    BK_UNUSED_VAR(delta_h);

    client_rect_ = compute_client_rect_();

    gui_state_->redraw();
}

void gui::window::move_to(scalar_t x, scalar_t y) {
    widget_base_t::move_to(x, y);

    client_rect_ = compute_client_rect_();

    gui_state_->redraw();
}

////////////////////////////////////////////////////////////////////////////////
// label
////////////////////////////////////////////////////////////////////////////////
gui::label::label(rect_t r, bklib::utf8string text)
    : widget_base_t(r)
    , text_(std::move(text))
{
}

void
gui::label::draw(renderer_t& renderer) const {
    //auto& b = renderer.get_solid_brush();

    //renderer.draw_rect(bounding_rect_, b);
    //renderer.draw_text(bounding_rect_, text_);
}

////////////////////////////////////////////////////////////////////////////////
// input box
////////////////////////////////////////////////////////////////////////////////
gui::input::input(rect_t r)
    : widget_base_t(r)
    , text_("Input")
    , composition_start_(0)
    , composition_end_(0)
{
}

void gui::input::draw(renderer_t& renderer) const {
    //auto& b = renderer.get_solid_brush();

    //b.set_color(gfx2d::color(0.8f, 0.8f, 0.8f));
    //renderer.fill_rect(bounding_rect_, b);

    //b.set_color(gfx2d::color(0.0f, 0.0f, 0.0f));
    //renderer.draw_rect(bounding_rect_, b);
    //renderer.draw_text(bounding_rect_, text_);
}

void gui::input::on_mouse_down(unsigned button) {
    gui_state_->capture_input_focus(this);
    widget_base_t::on_mouse_down(button);
}

void gui::input::on_input_char(bklib::utf32codepoint codepoint) {
    typedef
    std::wstring_convert<
        std::codecvt_utf8<bklib::utf32codepoint>, bklib::utf32codepoint
    > convert_t;

    switch (codepoint) {
        case 0x08 :
            while (!text_.empty()) {
                auto const c = text_.back();
                auto const mask0 = c & 0xC0;
                auto const mask1 = c & 0x80;

                text_.pop_back();

                if (mask1 == 0)         break;
                else if (mask0 != 0x80) break;
            }
            break;
        default : {
            auto result = convert_t().to_bytes(codepoint);
            text_.append(std::begin(result), std::end(result));
        }
    }

    gui_state_->redraw();
}

void gui::input::on_input_update_composition(
    bklib::input::ime::composition::range_list&& ranges
) {
    utf8string temp;

    for (auto const& i : ranges) {
        temp.append(i.text);
    }

    text_.replace(
        text_.begin() + composition_start_,
        text_.begin() + composition_end_,
        temp
    );

    composition_end_ = static_cast<unsigned>(text_.size());

    gui_state_->redraw();
}

void gui::input::on_input_begin_composition() {
    auto const pos = static_cast<unsigned>(text_.size());
    
    composition_start_ = pos;
    composition_end_   = pos;
}

void gui::input::on_input_end_composition() {
}

bklib::flagset<gui::input::focus_flags>
gui::input::on_focus_gained() {
    gui_state_->ime_set_text(this->text_);
    return focus_flags::want_text_input();
}

void gui::input::on_focus_lost() {
}

////////////////////////////////////////////////////////////////////////////////
// ime_candidate_list
////////////////////////////////////////////////////////////////////////////////
gui::ime_candidate_list::ime_candidate_list(rect_t r, bool visible)
    : widget_base_t(r)
    , visible_(visible)
{
}

gui::ime_candidate_list::ime_candidate_list()
    : ime_candidate_list(rect_t(0, 0, 100, 20*9), false)
{
}

void gui::ime_candidate_list::draw(renderer_t& renderer) const {
    //static scalar_t const scrollbar_padding = 3.0f;
    //static scalar_t const scrollbar_width   = 16.0f;
    //static scalar_t const selection_height  = 20.0f;

    //if (!visible_) {
    //    return;
    //}
    //   
    //auto const& window_rect = bounding_rect_;
    //   
    //rect const scrollbar_rect = rect(
    //    window_rect.right - scrollbar_width, window_rect.top,
    //    window_rect.right,                   window_rect.bottom
    //);

    //auto const scrollbar_indicator_height = scrollbar_rect.height() / candidates_.page_count();

    //rect const scrollbar_indicator_rect = rect(
    //    scrollbar_rect.left  + scrollbar_padding,
    //    scrollbar_rect.top   + scrollbar_indicator_height * (candidates_.page() + 0) + scrollbar_padding,
    //    scrollbar_rect.right - scrollbar_padding,
    //    scrollbar_rect.top   + scrollbar_indicator_height * (candidates_.page() + 1) - scrollbar_padding
    //);

    //rect label_rect = rect(
    //    window_rect.left,
    //    window_rect.top,
    //    window_rect.left + selection_height,
    //    window_rect.top  + selection_height
    //);

    //rect candidate_rect = rect(
    //    label_rect.right,
    //    window_rect.top,
    //    window_rect.right - scrollbar_rect.width(),
    //    window_rect.top   + selection_height
    //);

    //rect selection_rect = rect(
    //    window_rect.left,
    //    window_rect.top,
    //    window_rect.right - scrollbar_rect.width(),
    //    window_rect.top   + selection_height
    //);

    //auto& brush = renderer.get_solid_brush();

    //// draw background
    //brush.set_color(default_colors::window);
    //renderer.fill_rect(window_rect, brush);
    //    
    //brush.set_color(default_colors::highlight);
    //renderer.fill_rect(scrollbar_rect, brush);
    //    
    //brush.set_color(default_colors::window);
    //renderer.fill_rect(scrollbar_indicator_rect, brush);

    //brush.set_color(default_colors::text);

    //renderer.push_clip_rect(window_rect);

    //unsigned index = 0;
    //std::for_each(candidates_.page_begin(), candidates_.page_end(), [&](utf8string const& i) {
    //    if (index == candidates_.page_offset()) {
    //        brush.set_color(default_colors::highlight);
    //        renderer.fill_rect(selection_rect, brush);
    //        brush.set_color(default_colors::text);
    //    }

    //    renderer.draw_text(candidate_rect, i);
    //        
    //    char const label[] = {static_cast<char>('1' + index), 0}; //! HACK ish
    //    renderer.draw_text(label_rect, label);

    //    selection_rect.translate_y(selection_height);
    //    candidate_rect.translate_y(selection_height);
    //    label_rect.translate_y(selection_height);

    //    index++;
    //});

    //renderer.pop_clip_rect();
}
