#pragma once

#include <memory>
#include <vector>
#include <array>

#include <boost/circular_buffer.hpp>

#include "util/assert.hpp"
#include "types.hpp"
#include "common/math.hpp"
#include "gfx/renderer2d.hpp"

namespace bklib {
namespace gui {

////////////////////////////////////////////////////////////////////////////////
// Common types.
////////////////////////////////////////////////////////////////////////////////
typedef int16_t                scalar;
typedef math::rect<scalar>     rect;
typedef math::point<scalar, 2> point;
typedef math::range<scalar>    range;
typedef gfx::renderer2d        renderer;
typedef gfx::color<uint8_t, 4> color;

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
struct mouse_state_record;
class mouse_history;

struct mouse_listener;
struct keyboard_listener;
struct text_listener;

class state;
class root;
class widget_base;
class window;

//==============================================================================
//! Contains a snapshop of the mouse state.
//! std::is_pod -> true.
//==============================================================================
struct mouse_state_record {
    static size_t const BUTTON_COUNT = 5;

    typedef signed                         position_t;
    typedef std::array<bool, BUTTON_COUNT> buttons_t;

    void set_button(size_t i) {
        BK_ASSERT(i < BUTTON_COUNT);
        buttons[i] = true;
    }

    void clear_button(size_t i) {
        BK_ASSERT(i < BUTTON_COUNT);
        buttons[i] = false;
    }

    bool is_down(size_t i) const {
        BK_ASSERT(i < BUTTON_COUNT);
        return buttons[i];
    }

    bool is_up(size_t i) const {
        BK_ASSERT(i < BUTTON_COUNT);
        return !buttons[i];
    }

    point position() const {
        return point(x, y);
    }

    position_t x;
    position_t y;
    position_t scroll;
    buttons_t  buttons;
};

//==============================================================================
//! A circular buffer of the last HISTORY_SIZE mouse states: old records are
//! overwritten by new ones.
//==============================================================================
class mouse_history {
public:
    static unsigned const HISTORY_SIZE = 16;

    typedef mouse_state_record const& const_reference;
    
    mouse_history()
        : history_(HISTORY_SIZE)
    {
        mouse_state_record const rec = {0, 0, 0, {false}};
        push(rec);
        push(rec);
    }

    void push(mouse_state_record state) {
        history_.push_front(state);   
    }

    const_reference current() const {
        return history_[0];
    }

    const_reference last() const {
        return history_[1];
    }

    const_reference operator[](size_t i) const {
        return history_[i];
    }
private:
    typedef boost::circular_buffer<mouse_state_record> mouse_records_t_;

    mouse_records_t_ history_;
};

//==============================================================================
//! Interface for widgets that want mouse events.
//==============================================================================
struct mouse_listener {
    typedef signed   value_t;
    typedef unsigned button_t;
    
    virtual void on_mouse_move(value_t dx, value_t dy) = 0;
    virtual void on_mouse_move_to(value_t x, value_t y) = 0;
    virtual void on_mouse_scroll(value_t scroll) = 0;
    virtual void on_mouse_down(button_t button) = 0;
    virtual void on_mouse_up(button_t button) = 0;
    
    virtual void on_mouse_click(button_t button) = 0;
    virtual void on_mouse_double_click(button_t button) = 0;
    virtual void on_mouse_enter() = 0;
    virtual void on_mouse_leave() = 0;
    virtual void on_mouse_hover() = 0;

    virtual void on_gain_mouse_capture() = 0;
    virtual void on_lose_mouse_capture() = 0;
};

//==============================================================================
//! Interface for widgets that want keyboard events.
//==============================================================================
struct keyboard_listener {
    typedef unsigned key_code;

    virtual void on_key_down(key_code key) = 0;
    virtual void on_key_up(key_code key) = 0;
    virtual void on_key_repeat(key_code key) = 0;

    virtual void on_gain_keyboard_capture() = 0;
    virtual void on_lose_keyboard_capture() = 0;
};

//==============================================================================
//! Interface for widgets that want text input events.
//==============================================================================
struct text_listener {
    virtual void on_gain_text_capture() = 0;
    virtual void on_lose_text_capture() = 0;
};

//==============================================================================
//! Shared gui state accessible to all widgets.
//==============================================================================
class state {
    friend root; // logically state is part of root.
public:
    typedef mouse_listener*    mouse_listener_t;
    typedef keyboard_listener* keyboard_listener_t;
    typedef text_listener*     text_listener_t;
public:
    explicit state(renderer& r);

    //--------------------------------------------------------------------------
    //! Take exclusive control over an event classification.
    //! Sends on_lose_capture_* to the current listener.
    //! Sends on_gain_capture_* to the new listener.
    //! @pre listener must be a valid object.
    //! @pre listener must not currently have exclusive control.
    //--------------------------------------------------------------------------
    template <typename T>
    void capture(T listener) {
        auto& current = listener_(listener);

        BK_ASSERT(listener != nullptr);
        BK_ASSERT(current != listener);

        if (current) on_lose_capture_(current);
        current = listener;
        on_gain_capture_(current);
    }

    //--------------------------------------------------------------------------
    //! Release exclusive control over an event classification.
    //! Sends on_lose_capture_* to the current listener.
    //! @pre listener must have exclusive control currently.
    //--------------------------------------------------------------------------
    template <typename T>
    void release(T listener) {
        auto& current = listener_(listener);

        BK_ASSERT(current == listener);
        BK_ASSERT(current != nullptr);

        on_lose_capture_(current);
        current = nullptr;
    }

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    mouse_history const& get_mouse_history() const {
        return mouse_history_;
    }

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    template <typename T>
    T listener() { return listener_((T)nullptr); }
    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    template <typename T>
    T const listener() const { return listener_((T)nullptr); }

    renderer&       get_renderer()       { return renderer_; }
    renderer const& get_renderer() const { return renderer_; }
private:
    state(state const&); //=delete
    state& operator=(state const&); //=delete

    mouse_listener_t&    listener_(mouse_listener_t)    { return mouse_listener_; }
    keyboard_listener_t& listener_(keyboard_listener_t) { return keyboard_listener_; }
    text_listener_t&     listener_(text_listener_t)     { return text_listener_; }

    void on_lose_capture_(mouse_listener_t l)    { l->on_lose_mouse_capture(); }
    void on_lose_capture_(keyboard_listener_t l) { l->on_lose_keyboard_capture(); }
    void on_lose_capture_(text_listener_t l)     { l->on_lose_text_capture(); }

    void on_gain_capture_(mouse_listener_t l)    { l->on_gain_mouse_capture(); }
    void on_gain_capture_(keyboard_listener_t l) { l->on_gain_keyboard_capture(); }
    void on_gain_capture_(text_listener_t l)     { l->on_gain_text_capture(); }

    void push_mouse_record_(mouse_state_record const& rec) {
        mouse_history_.push(rec);
    }
private:
    renderer&           renderer_;

    mouse_listener_t    mouse_listener_;
    keyboard_listener_t keyboard_listener_;
    text_listener_t     text_listener_;

    mouse_history       mouse_history_;
};

//==============================================================================
//! Root of the gui system; all widgets must be created by and added to it.
//==============================================================================
class root {
public:
    typedef mouse_listener::value_t      value_t;
    typedef mouse_listener::button_t     button_t;
    typedef std::unique_ptr<widget_base> child;

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------    
    explicit root(renderer& renderer);

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    // broken in msvc 2012 ctp
    //template <typename T, typename... Args>
    //std::unique_ptr<T> make_widget(Args&&... args) {
    //    return std::make_unique<T>(state_, std::forward<Args>(args)...);
    //}

    //--------------------------------------------------------------------------
    //! Used to create all gui widgets. Sets the widget's internal state ref.
    //--------------------------------------------------------------------------
    template <typename T>
    std::unique_ptr<T> make_widget() {
        return std::make_unique<T>(state_);
    }

    template <typename T, typename A0>
    std::unique_ptr<T> make_widget(A0&& a0) {
        return std::make_unique<T>(state_, std::forward<A0>(a0));
    }

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    void draw() const;
    
    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------
    template <typename T>
    void add_widget(std::unique_ptr<T> w) {
        static_assert(
            std::is_base_of<widget_base, T>::value,
            "type not derived from widget_base"
        );

        add_widget_(child(w.release()));
    }

    void on_mouse_move(value_t dx, value_t dy);
    void on_mouse_move_to(value_t x, value_t y);
    void on_mouse_scroll(value_t scroll);
    void on_mouse_down(button_t button);
    void on_mouse_up(button_t button);
private:
    root(root const&); //=delete
    root const& operator=(root const&); //=delete

    typedef std::vector<child> container_t_;
    typedef container_t_::iterator iterator_;
    typedef container_t_::const_iterator const_iterator_;

    //--------------------------------------------------------------------------
    //!
    //--------------------------------------------------------------------------    
    void add_widget_(child widget);

    iterator_ get_topmost_widget_at_(value_t x, value_t y);
    void send_to_top_(iterator_ where);

    container_t_ children_;
    state        state_;
};

//==============================================================================
//! Base type from which all widgets are derived.
//==============================================================================
class widget_base : public mouse_listener {
public:
    widget_base() { }
    virtual ~widget_base() { }

    virtual void draw() const = 0;

    virtual rect const& get_rect() const = 0;
    virtual void set_rect(rect const& r) = 0;
private:
    widget_base(widget_base const&); //=delete
    widget_base& operator=(widget_base const&); //=delete
};

//==============================================================================
//!
//==============================================================================
class window : public widget_base {
public:
    static unsigned const BORDER_SIZE = 5;
    static unsigned const TITLE_SIZE  = 20;

    //! @param r The client rect.
    static rect get_non_client_rect(rect const& r);
    //! @param r The non-client rect.
    static rect get_client_rect(rect const& r);
    //! @param r The non-client rect.
    static rect get_title_rect(rect const& r);

    window(state& state, rect const& r);

    virtual void draw() const override;

    virtual rect const& get_rect() const override;
    virtual void set_rect(rect const& r) override;

    virtual void translate(scalar dx, scalar dy);
    
    virtual void resize(rect::side_x sx, scalar dx, rect::side_y sy, scalar dy);

    virtual void on_mouse_move(value_t dx, value_t dy) override;
    virtual void on_mouse_move_to(value_t x, value_t y) override;
    virtual void on_mouse_scroll(value_t scroll) override;
    virtual void on_mouse_down(button_t button) override;
    virtual void on_mouse_up(button_t button) override;
    
    virtual void on_mouse_click(button_t button) override;
    virtual void on_mouse_double_click(button_t button) override;
    virtual void on_mouse_enter() override;
    virtual void on_mouse_leave() override;
    virtual void on_mouse_hover() override;

    virtual void on_gain_mouse_capture() override;
    virtual void on_lose_mouse_capture() override;
private:
    enum class window_state {
        none, sizing, moving,
    };
private:
    state& state_;
    
    rect non_client_rect_;
    rect client_rect_;
    rect title_rect_;

    renderer::handle non_client_rect_handle_;
    renderer::handle client_rect_handle_;

    window_state              window_state_;
    math::border_intersection sizing_side_;
};

} // namespace gui
} // namespace bklib
