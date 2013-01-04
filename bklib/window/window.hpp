//------------------------------------------------------------------------------
//! @file
//! @author Brandon Kentel
//! @date   Dec 2012
//! @brief  System window management.
//------------------------------------------------------------------------------
#pragma once

#include "util/callback.hpp"
#include "common/math.hpp"

namespace bklib { namespace input { namespace ime {
    // forward declaration for promise
    class manager;
}}}

namespace bklib {
//------------------------------------------------------------------------------
//! System "window" creation, management, and manipulation.
//! Implementation is platform dependant; hence the pimpl idiom.
//------------------------------------------------------------------------------
class window {
public:
    //! platform specific system window handle type.
    typedef HWND                              handle_t;
    typedef unsigned                          key_code_t;
    typedef math::range<unsigned>             range_t;
    typedef std::promise<
        std::shared_ptr<input::ime::manager>> promise_t;

    //--------------------------------------------------------------------------
    //! Constructs a rendering window controlled by a seperate thread.
    //! @param finished
    //!     A promise to be fulfilled after the thread has begun execution.
    //--------------------------------------------------------------------------
    window(promise_t& finished);

    //--------------------------------------------------------------------------
    //! Set the min and max size constraints for the window.
    //! @param height The [min, max] range for height.
    //! @param width  The [min, max] range for width.
    //--------------------------------------------------------------------------
    void set_min_max_size(range_t height, range_t width);

    //! Show or hide the window.
    void show(bool visible = true);

    //--------------------------------------------------------------------------
    //! Check if there are events waiting to be processed.
    //! @returns
    //!     @c true if there are events waiting, @c false otherwise
    //--------------------------------------------------------------------------
    bool has_pending_events() const;
    
    //! Do all pending events without blocking.
    void do_pending_events();

    //! Do a pending event, block otherwise.
    void do_event_wait();

    //! Request the window close.
    void close();

    //! Wait for the thread to finish and join.
    void wait();

    //! Returns the underlying window handle.
    handle_t handle() const;

    //--------------------------------------------------------------------------
    BK_UTIL_CALLBACK_BEGIN;
        //----------------------------------------------------------------------
        //! Called when a keyboard key goes down.
        //! @li @a key the keycode.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_key_down,
                                  void (key_code_t key) );
        //----------------------------------------------------------------------
        //! Called when a keyboard key goes up.
        //! @li @a key the keycode.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_key_up,
                                  void (key_code_t key) );
        //----------------------------------------------------------------------
        //! Called when the user has requested the window be closed.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_close,
                                  void () );
        //----------------------------------------------------------------------
        //! Called when the window must be repainted.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_paint,
                                  void () );
        //----------------------------------------------------------------------
        //! Called when the window has been resized.
        //! @li @a w
        //!     The new width.
        //! @li @a h
        //!     the new height.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_size,
                                  void (unsigned w, unsigned h) );
        //----------------------------------------------------------------------
        //! Called when the mouse has been moved. The point (x, y) is relative
        //! to the upper left hand corner of the window (0, 0).
        //! @li @a x the new x position.
        //! @li @a y the new y position.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_mouse_move_to,
                                  void (int x, int y) );
        //----------------------------------------------------------------------
        //! Called when the mouse has been moved. The pait (dx, dy) indicates
        //! the absolute displacement since the last event.
        //! @li @a dx
        //!     Displacement on the x axis.
        //! @li @a dy
        //!     Displacement on the y axis.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_mouse_move,
                                  void (int dx, int dy) );
        //----------------------------------------------------------------------
        //! Called when a mouse button goes down.
        //! @li @a button
        //!     Index of the button that went down.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_mouse_down,
                                  void (unsigned button) );
        //----------------------------------------------------------------------
        //! Called when a mouse button goes up.
        //! @li @a button
        //!     Index of the button that went up.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_mouse_up,
                                  void (unsigned button) );
        //----------------------------------------------------------------------
        //! Called when the mouse wheel is scrolled.
        //! @li @a ds
        //!     The one dimensional scroll value in device units.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_mouse_scroll,
                                  void (int ds) );
        //----------------------------------------------------------------------
        //! Called when a single character of input is received.
        //! @li @a code
        //!     The utf-8 encoding of the character input.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_input_char,
                                  void (utf32codepoint code) );
        //----------------------------------------------------------------------
    BK_UTIL_CALLBACK_END;
    //--------------------------------------------------------------------------
public:
    //! required for pimpl to work correctly.
    ~window();
private:
    //! Implementation defined elsewhere.
    struct impl_t;
    //! Opaque implementation.
    std::unique_ptr<impl_t> const impl_;
}; //class window

BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_key_down);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_key_up);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_close);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_paint);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_size);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_mouse_move_to);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_mouse_move);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_mouse_down);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_mouse_up);
BK_UTIL_CALLBACK_DECLARE_EXTERN(window, on_mouse_scroll);
//------------------------------------------------------------------------------

} // namesapce bklib
