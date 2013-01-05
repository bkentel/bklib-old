#pragma once

#include "gfx/renderer/renderer2d/renderer2d.hpp"
#include "common/math.hpp"
#include "input/input.hpp"
#include "util/cache.hpp"
#include "util/callback.hpp"
#include "util/flagset.hpp"

namespace bklib { namespace gui {

typedef float                    scalar_t;
typedef math::rect<scalar_t>     rect;
typedef math::point<scalar_t, 2> point;
typedef math::range<scalar_t>    range;

typedef gfx2d::color      color;
typedef gfx2d::renderer   renderer_t;

typedef bklib::window::key_code_t key_code_t;
typedef std::shared_ptr<bklib::input::ime::manager> shared_manager;
typedef bklib::input::ime::composition::range_list range_list;

class widget_base_t;
class parent_base_t;
class root;
class ime_candidate_list;

namespace default_colors {
    extern color const window;
    extern color const border;
    extern color const text;
    extern color const highlight;
} //namespace colors

//------------------------------------------------------------------------------
//! A record of mouse state.
//------------------------------------------------------------------------------
struct mouse_state {
    typedef signed position;

    //! Hard coded based on the windows raw input API
    static unsigned const BUTTON_COUNT = 5;

    //! Mouse button state.
    enum class button_state : unsigned {
        up,   //<! Mouse button is up.
        down, //<! Mouse button is down.
    };

    //! Initialize all buttons to @c up
    mouse_state(position x, position y) : x(x), y(y) {
        buttons.fill(button_state::up);
    }

    //! Initialize to (0, 0) and all buttons to @c up
    mouse_state() : mouse_state(0, 0) { }

    mouse_state(mouse_state const& other)
        : x(other.x), y(other.y), buttons(other.buttons) { }

    mouse_state(unsigned button, button_state state) : mouse_state() {
        BK_ASSERT_MSG(button < BUTTON_COUNT, "Out of range.");
        buttons[button] = state;
    }

    void set(position x, position y) {
        this->x = x;
        this->y = y;
    }

    position x, y;
    std::array<button_state, BUTTON_COUNT> buttons;
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! State and functions global to all objects owned by a root object.
//------------------------------------------------------------------------------
class gui_state {
    friend class root;
public:
    typedef widget_base_t*        pointer;
    typedef mouse_state::position mouse_position;

    //! Number of past mouse records to keep
    static unsigned const MOUSE_HISTORY_SIZE = 16;

    //! register as wanting to receive text input messages
    void capture_input_focus(pointer p);
    //! unregister as wanting to receive text input messages
    void release_input_focus(pointer p);

    //! register as wanting to trace the mouse
    void capture_mouse_input(pointer p);
    //! unregister as wanting to trace the mouse
    void release_mouse_input(pointer p);

    //! current mouse x coord
    mouse_position mouse_x() const { return mouse_state_.x; }
    //! current mouse y coord
    mouse_position mouse_y() const { return mouse_state_.y; }
    //! previous mouse x coord
    mouse_position mouse_last_x() const { return mouse_history_.back().x; }
    //! previous mouse y coord
    mouse_position mouse_last_y() const { return mouse_history_.back().y; }

    //! Request that the root redraw itself.
    void redraw() {
        if (on_redraw_) on_redraw_();
    }

    void ime_cancel_composition();
    void ime_set_text(utf8string const& string);
private:
    typedef boost::circular_buffer<mouse_state> buffer_t;

    gui_state(shared_manager manager);

    void on_mouse_move_to_(signed x, signed y);
    void on_mouse_button_state_(unsigned button, mouse_state::button_state state);

    void set_ime_manager_(shared_manager manager);
    
    //--------------------------------------------------------------------------
    //! Pointer to the widget which has input focus.
    pointer input_focus_listener_;
    //! Pointer to the widget which has mouse focus.
    pointer mouse_input_listener_;

    mouse_state mouse_state_;
    buffer_t mouse_history_;

    shared_manager ime_manager_;

    std::function<void ()> on_redraw_;
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Default implementation for widgets that act as containers for other widgets.
//------------------------------------------------------------------------------
class parent_base_t {
public:
    typedef widget_base_t           child_t;
    typedef bklib::cache_t<child_t> container_t;
    typedef container_t::handle_t   handle_t;
    typedef container_t::unique_t   unique_t;
    //--------------------------------------------------------------------------
    parent_base_t(size_t reserve = 0);
    virtual ~parent_base_t() {}
    //--------------------------------------------------------------------------
    virtual handle_t add_child(unique_t child);
    virtual unique_t remove_child(handle_t handle);

    virtual child_t&       get_child(handle_t handle);
    virtual child_t const& get_child(handle_t handle) const;
    //--------------------------------------------------------------------------
    BK_UTIL_CALLBACK_BEGIN;
        BK_UTIL_CALLBACK_DECLARE( on_child_add,
                                  void (parent_base_t& p, child_t& c) );
        BK_UTIL_CALLBACK_DECLARE( on_child_remove,
                                  void (parent_base_t& p, child_t& c) );
    BK_UTIL_CALLBACK_END;
protected:
    container_t children_;

    event_on_child_add::type    callback_on_child_add_;
    event_on_child_remove::type callback_on_child_remove_;
}; //---------------------------------------------------------------------------

BK_UTIL_CALLBACK_DECLARE_EXTERN(parent_base_t, on_child_add);
BK_UTIL_CALLBACK_DECLARE_EXTERN(parent_base_t, on_child_remove);

//------------------------------------------------------------------------------
//! Interface for widgets that want text input.
//------------------------------------------------------------------------------
class input_listener {
public:
    typedef bklib::input::ime::composition::range_list range_list;

    virtual ~input_listener() { }

    virtual void on_input_char(utf32codepoint codepoint);
    virtual void on_input_update_composition(range_list&& ranges);
    virtual void on_input_begin_composition();
    virtual void on_input_end_composition();

}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Interface for widgets that want keyboard input.
//------------------------------------------------------------------------------
class keyboard_listener {
public:
    virtual void on_key_down();
    virtual void on_key_up();
    virtual void on_key_repeat();
}; //---------------------------------------------------------------------------


//------------------------------------------------------------------------------
//! Default implementation for simple widgets.
//------------------------------------------------------------------------------
class widget_base_t {
public:
    typedef math::rect_base::side_x side_x;
    typedef math::rect_base::side_y side_y;
    //--------------------------------------------------------------------------
    explicit widget_base_t(rect r);
    virtual ~widget_base_t();
    //--------------------------------------------------------------------------
    virtual void set_bounding_rect(rect r);
    virtual rect get_bounding_rect() const;

    virtual void set_gui_state(gui_state& state);

    virtual void on_set_input_focus() {
        gui_state_->capture_input_focus(this);
    }

    virtual void resize(
        scalar_t dw,
        scalar_t dh,
        side_x sx = side_x::right,
        side_y sy = side_y::bottom
    );

    virtual void translate(scalar_t dx, scalar_t dy);
    virtual void move_to(scalar_t x, scalar_t y);

    virtual bool hit_test(scalar_t x, scalar_t y) const;
    virtual bool hit_test(point p) const;

    virtual void draw(renderer_t& renderer) const;
    //--------------------------------------------------------------------------
    virtual void on_mouse_enter();
    virtual void on_mouse_leave();
    virtual void on_mouse_down(unsigned button);
    virtual void on_mouse_up(unsigned button);
    virtual void on_mouse_click(unsigned button);
    virtual void on_mouse_move(unsigned x, unsigned y, signed dx, signed dy);
    //--------------------------------------------------------------------------
    virtual void on_input_char(utf32codepoint codepoint);
    //--------------------------------------------------------------------------
    virtual void on_input_update_composition(
        range_list&& //ranges
    ) { }
    virtual void on_input_begin_composition() { }
    virtual void on_input_end_composition() { }
    //--------------------------------------------------------------------------
    struct focus_flags {
        typedef flagset_flag<focus_flags, 0> want_text_input;
        typedef flagset_flag<focus_flags, 1> want_kb_input;
        typedef flagset_flag<focus_flags, 2> want_mouse_input;
    };
    
    virtual flagset<focus_flags> on_focus_gained() { return flagset<focus_flags>(0); }
    virtual void on_focus_lost()   {}
    //--------------------------------------------------------------------------
    BK_UTIL_CALLBACK_BEGIN;
        BK_UTIL_CALLBACK_DECLARE(on_mouse_enter, void (widget_base_t& w));
        BK_UTIL_CALLBACK_DECLARE(on_mouse_leave, void (widget_base_t& w));
        BK_UTIL_CALLBACK_DECLARE(on_mouse_down,  void (widget_base_t& w, unsigned button));
        BK_UTIL_CALLBACK_DECLARE(on_mouse_up,    void (widget_base_t& w, unsigned button));
        BK_UTIL_CALLBACK_DECLARE(on_mouse_click, void (widget_base_t& w, unsigned button));
        BK_UTIL_CALLBACK_DECLARE(on_mouse_move,  void (widget_base_t& w, unsigned x, unsigned y, signed dx, signed dy));

        BK_UTIL_CALLBACK_DECLARE(on_resize, bool (widget_base_t& w, rect new_bounds, rect old_bounds));
        BK_UTIL_CALLBACK_DECLARE(on_move,   bool (widget_base_t& w, rect new_bounds, rect old_bounds));

        BK_UTIL_CALLBACK_DECLARE(on_input_char, void (widget_base_t& w, utf32codepoint code));
    BK_UTIL_CALLBACK_END;
protected:
    gui_state* gui_state_;
    rect       bounding_rect_;

    event_on_mouse_enter::type callback_on_mouse_enter_;
    event_on_mouse_leave::type callback_on_mouse_leave_;
    event_on_mouse_down::type  callback_on_mouse_down_;
    event_on_mouse_up::type    callback_on_mouse_up_;
    event_on_mouse_click::type callback_on_mouse_click_;
    event_on_mouse_move::type  callback_on_mouse_move_;

    event_on_resize::type      callback_on_resize_;
    event_on_move::type        callback_on_move_;

    event_on_input_char::type  callback_on_input_char_;
};

BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_enter);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_leave);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_down);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_up);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_click);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_mouse_move);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_resize);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_move);
BK_UTIL_CALLBACK_DECLARE_EXTERN(widget_base_t, on_input_char);

////////////////////////////////////////////////////////////////////////////////
// A list to display IME candidates
////////////////////////////////////////////////////////////////////////////////
class ime_status_indicator : public widget_base_t {
public :
    ime_status_indicator();
};

////////////////////////////////////////////////////////////////////////////////
// A list to display IME candidates
////////////////////////////////////////////////////////////////////////////////
class ime_candidate_list : public widget_base_t {
public:
    ime_candidate_list(rect r, bool visible = false);
    ime_candidate_list();

    virtual void draw(renderer_t& renderer) const;

    void set_items_per_page(unsigned n) {
        candidates_.set_items_per_page(n);
    }

    void set_current_page(unsigned p) {
        candidates_.set_page(p);
    }

    void set_current_selection(unsigned i) {
        candidates_.set_sel(i);
    }

    void set_strings(std::vector<utf8string>&& strings) {
        candidates_.set_items(std::move(strings));
    }

    void show(bool b = true) {
        visible_ = b;
    }
private:
    bool visible_;
    bklib::input::ime::candidate_list candidates_;
};

//------------------------------------------------------------------------------
//! Root of the gui system.
//------------------------------------------------------------------------------
class root : public parent_base_t {
public:
    explicit root(shared_manager manager);

    void draw(renderer_t& renderer) const;

    handle_t add_child(unique_t child) override;
    unique_t remove_child(handle_t handle) override;

    void on_mouse_move(int dx, int dy);
    void on_mouse_move_to(int x, int y);
    void on_mouse_down(unsigned button);
    void on_mouse_up(unsigned button);

    void on_input_char(utf32codepoint codepoint);

    void on_key_up(key_code_t key);
    void on_key_down(key_code_t key);

    BK_UTIL_CALLBACK_BEGIN;
        BK_UTIL_CALLBACK_DECLARE( on_update, void () );
    BK_UTIL_CALLBACK_END;
private:
    typedef std::list<widget_base_t*> zorder_t;
    zorder_t zorder_;

    gui_state gui_state_;
    ime_candidate_list ime_candidate_list_;
};

BK_UTIL_CALLBACK_DECLARE_EXTERN(root, on_update);

////////////////////////////////////////////////////////////////////////////////
// Simple window widget.
////////////////////////////////////////////////////////////////////////////////
class window
    : public widget_base_t
    , public parent_base_t
{
public:
    static unsigned const BORDER_SIZE = 6;
    static unsigned const HEADER_SIZE = 24;

    explicit window(rect r);
    virtual ~window() { }

    virtual void move_to(scalar_t x, scalar_t y) override;

    virtual void resize(
        scalar_t dw,
        scalar_t dh,
        side_x sx = side_x::right,
        side_y sy = side_y::bottom
    ) override;

    virtual void on_mouse_enter()                     override;
    virtual void on_mouse_leave()                     override;
    virtual void on_mouse_down(unsigned button)       override;
    virtual void on_mouse_up(unsigned button)         override;
    virtual void on_mouse_move(
        unsigned x, unsigned y, signed dx, signed dy) override;

    virtual void draw(renderer_t& renderer) const override;

    virtual rect const& get_client_rect() const;

    virtual void set_gui_state(gui_state& state) override;
private:
    scalar_t compute_minimum_width_() const {
        return BORDER_SIZE * 2;
    }

    scalar_t compute_minimum_height_() const {
        return BORDER_SIZE + HEADER_SIZE;
    }

    rect compute_header_rect_() const {
        auto const r = get_bounding_rect();
        return rect(r.left, r.top, r.right, r.top + HEADER_SIZE);
    }

    rect compute_client_rect_() const {
        auto const r = get_bounding_rect();
        return rect(
            r.left   + BORDER_SIZE,
            r.top    + HEADER_SIZE,
            r.right  - BORDER_SIZE,
            r.bottom - BORDER_SIZE
        );
    }

    gfx::color_code back_color_;
    gfx::color_code border_color_;
    gfx::color_code title_color_;
    gfx::color_code text_color_;

    enum class state {
        none,
        moving,
        sizing,
    };

    struct sizing_info {
        sizing_info(
            side_x x = side_x::none,
            side_y y = side_y::none
        ) : x(x), y(y) { }

        side_x x;
        side_y y;
    };

    state       state_;
    sizing_info sizing_info_;

    range width_constraint_;
    range height_constraint_;

    rect client_rect_;
};

////////////////////////////////////////////////////////////////////////////////
// A basic input field
////////////////////////////////////////////////////////////////////////////////
class input : public widget_base_t {
public:
    input(rect r);
    virtual ~input() { }

    virtual void on_input_char(utf32codepoint codepoint);

    virtual void on_input_update_composition(range_list&& ranges) override;
    virtual void on_input_begin_composition()  override;
    virtual void on_input_end_composition()  override;

    virtual void on_mouse_down(unsigned button);

    virtual void draw(renderer_t& renderer) const;

    virtual flagset<focus_flags> on_focus_gained() override;
    virtual void on_focus_lost()   override;
private:
    utf8string text_;
    unsigned composition_start_;
    unsigned composition_end_;
};

////////////////////////////////////////////////////////////////////////////////
// A simple list
////////////////////////////////////////////////////////////////////////////////
class list : public widget_base_t {
public:
    list(rect r) : widget_base_t(r) { }
private:
    std::vector<utf8string> items_;
};


////////////////////////////////////////////////////////////////////////////////
// A simple text label
////////////////////////////////////////////////////////////////////////////////
class label : public widget_base_t {
public:

    label(rect r, utf8string text);
    virtual ~label() { }

    virtual void draw(renderer_t& renderer) const;
private:
    utf8string text_;
};

////////////////////////////////////////////////////////////////////////////////
// A simple push button
////////////////////////////////////////////////////////////////////////////////
class button : public widget_base_t {
public:
    button(rect r, utf8string text);
private:
    utf8string text_;
};


} //namespace gui
} //namespace bklib
