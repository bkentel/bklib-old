//------------------------------------------------------------------------------
//! @file
//! @author Brandon Kentel
//! @date   Dec 2012
//! @brief  Windows implementation of bklib::window.
//------------------------------------------------------------------------------

#include "pch.hpp"
#include "window/window.hpp"

namespace bklib { namespace detail { namespace impl {
//------------------------------------------------------------------------------
//! Windows specific implementation for bklib::window.
//------------------------------------------------------------------------------
struct window_impl {
    //! Default window style.
    window_impl();

    //! Actually create the window. This will also detach any console that may
    //! be present.
    void create();
    
    //! Destroy the window.
    void close();

    //! Show or hide the window.
    void show(bool visible);

    //! Do all ready events without blocking.
    bool do_pending_events();

    //! Do a ready event, block otherwise.
    bool do_event_wait();

    //! Returns the underlying window handle.
    HWND handle() const { return handle_; }

    //! Enable raw input messages for this window.
    void enable_raw_input();

    //! Get a user friendly name for a virtual keycode
    utf8string get_key_name(window::key_code_t key) const;

    //! Send a dummy message to wake any threads blocked on the message queue.
    void notfify() {
        ::PostMessage(handle_, WM_USER, 0, 0);
    }
    //--------------------------------------------------------------------------
    window::event_on_key_down::type      on_key_down;
    window::event_on_key_up::type        on_key_up;
    
    window::event_on_close::type         on_close;
    window::event_on_paint::type         on_paint;
    window::event_on_size::type          on_size;

    window::event_on_mouse_move_to::type on_mouse_move_to;
    window::event_on_mouse_move::type    on_mouse_move;
    window::event_on_mouse_down::type    on_mouse_down;
    window::event_on_mouse_up::type      on_mouse_up;
    window::event_on_mouse_scroll::type  on_mouse_scroll;

    window::event_on_input_char::type    on_input_char;
    //--------------------------------------------------------------------------

    //! Top level winproc called by windows.
    static LRESULT CALLBACK top_level_wnd_proc_(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //! Instance specific winproc.
    LRESULT window_proc_(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    struct result_t : public std::pair<bool, LRESULT> {
        result_t(bool processed, LRESULT value) : std::pair<bool, LRESULT>(processed, value) {}

        result_t() : result_t(false, 0) {}
        explicit result_t(LRESULT value) : result_t(true, value) { }

        explicit operator bool() const { return first; }
        explicit operator LRESULT() const { return second; }

        static result_t use_def_win_proc() { return result_t(); }
        static result_t return_value(LRESULT value) { return result_t(value); }
    };

    //--------------------------------------------------------------------------
    //! Windows message handler.
    //! @tparam msg
    //!     The windows message code.
    //! @param wParam As in windows.
    //! @param lParam As in windows.
    //--------------------------------------------------------------------------
    template <UINT msg>
    result_t handle_message(WPARAM wParam, LPARAM lParam);

    //! Create a HWND.
    static HWND create_window_(window_impl* window);

    //! The system window handle.
    HWND handle_;
private:
    window_impl(window_impl const&); //= delete
    window_impl& operator=(window_impl const&); //=delete
};
//------------------------------------------------------------------------------

} //namespace impl
} //namespace detail
} //namespace bklib

//------------------------------------------------------------------------------
namespace impl = ::bklib::detail::impl;

namespace {
    static wchar_t const CLASS_NAME[] = L"BKLIB_WIN";
} //namespace anon

//------------------------------------------------------------------------------
impl::window_impl::window_impl()
    : handle_(nullptr)
{
}

//------------------------------------------------------------------------------
void impl::window_impl::create()
{
    ::FreeConsole();

    handle_ = create_window_(this);
    enable_raw_input();

    ::ShowWindow(handle_, SW_SHOWDEFAULT);
    ::InvalidateRect(handle_, nullptr, FALSE);
    ::UpdateWindow(handle_);
}

//------------------------------------------------------------------------------
void impl::window_impl::close() {
    ::DestroyWindow(handle_); // ignore the return value
}

//------------------------------------------------------------------------------
void impl::window_impl::show(bool visible) {
    ::ShowWindow(handle_, visible ? SW_SHOW : SW_HIDE);
    ::InvalidateRect(handle_, nullptr, FALSE);
    ::UpdateWindow(handle_);
}

//------------------------------------------------------------------------------
bool impl::window_impl::do_pending_events() {
    MSG msg;

    while (::PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
        if (!do_event_wait()) {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
//! @return
//!     @c false if the window has been destroyed, @c true otherwise
//! @throw bklib::platform::windows_exception
//!     Thrown on any error.
//------------------------------------------------------------------------------
bool impl::window_impl::do_event_wait() {
    MSG msg;
    auto const result = ::GetMessageW(&msg, 0, 0, 0);

    if (result == 0) {
        return false;
    } else if (result == -1) {
        BK_THROW_ON_FAIL(::GetMessageW, result);
    }

    ::TranslateMessage(&msg);
    ::DispatchMessageW(&msg);

    return true;
}

namespace {
    //--------------------------------------------------------------------------
    //! Get the userdata for the window given by @c hwnd (our window object).
    //! @throw bklib::platform::windows_exception
    //--------------------------------------------------------------------------
    impl::window_impl* get_window_ptr(HWND hwnd) {
        ::SetLastError(0);
        auto const result = ::GetWindowLongPtrW(hwnd, GWLP_USERDATA);

        BK_THROW_ON_FAIL(::GetWindowLongPtrW,
            HRESULT_FROM_WIN32(::GetLastError()));

        return reinterpret_cast<impl::window_impl*>(result);
    }

    //--------------------------------------------------------------------------
    //! Set the userdata for the window given by @c hwnd to be our
    //! window object.
    //! @throw bklib::platform::windows_exception
    //--------------------------------------------------------------------------
    void set_window_ptr(HWND hwnd, impl::window_impl* ptr) {
        ::SetLastError(0);
        auto const result = ::SetWindowLongPtrW(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(ptr) );

        BK_THROW_ON_FAIL(::GetWindowLongPtrW,
            HRESULT_FROM_WIN32(::GetLastError()));

        BK_ASSERT_MSG(result == 0, "value was previously set");
    }
} //namespace

//------------------------------------------------------------------------------
//! Top level window procedure which forwards messages to the appropriate
//! impl::window_impl instance.
//! @throw noexcept
//!     Swallows all exceptions at the API boundary.
//------------------------------------------------------------------------------
LRESULT CALLBACK impl::window_impl::top_level_wnd_proc_(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam
) throw() try {   
    // set the instance pointer for the window given by hwnd if it was
    // just created.
    if (msg == WM_NCCREATE) {
        auto const cs =
            reinterpret_cast<CREATESTRUCTW const*>(lParam);
        auto const window_ptr =
            reinterpret_cast<window_impl*>(cs->lpCreateParams);

        set_window_ptr(hwnd, window_ptr);

        // disable legacy IME messages; we are TSF aware.
        auto const result = ::ImmDisableIME((DWORD)-1);
        BK_THROW_ON_FAIL(::ImmDisableIME, result);
    }

    // the window object to forward the message to.
    auto const window = get_window_ptr(hwnd);

    if (window) {
        return window->window_proc_(hwnd, msg, wParam, lParam);
    } else {
        // it's possible we will receive some messages beofre WM_NCCREATE;
        // use the default handler.
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }
} catch (std::exception&) {
    BK_TODO_BREAK; //! @todo implement
} catch (...) {
    BK_TODO_BREAK; //! @todo implement
}

//------------------------------------------------------------------------------
//! WM_PAINT specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_PAINT>(WPARAM, LPARAM) {
    if (on_paint) {
        on_paint();
        ::ValidateRect(handle_, nullptr);

        return result_t::return_value(0);
    }

    return result_t::use_def_win_proc();
}

//------------------------------------------------------------------------------
//! WM_SIZE specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_SIZE>(WPARAM, LPARAM) {
    if (on_size) {
        RECT rect;
        ::GetClientRect(handle_, &rect);

        unsigned const w = std::abs(rect.right  - rect.left);
        unsigned const h = std::abs(rect.bottom - rect.top);

        on_size(w, h);
        return result_t::return_value(0);
    }

    return result_t::use_def_win_proc();
}

//------------------------------------------------------------------------------
//! WM_MOUSEMOVE specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_MOUSEMOVE>(WPARAM, LPARAM lParam) {
    if (on_mouse_move_to) {
        auto const pos = MAKEPOINTS(lParam);

        on_mouse_move_to(pos.x, pos.y);
        return result_t::return_value(0);
    }

    return result_t::use_def_win_proc();
}

//------------------------------------------------------------------------------
//! WM_CHAR specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_CHAR>(WPARAM wParam, LPARAM) {
    auto const code = static_cast<wchar_t>(wParam & 0xFFFF); // utf-16

    if (code >= 0xD800 && code <= 0xDBFF) {
        BK_TODO_BREAK;
    } else if (code >= 0xDC00 && code <= 0xDFFF) {
        BK_TODO_BREAK;
    }

    if (on_input_char) {
        on_input_char(code);
        return result_t::return_value(0);
    }

    return result_t::use_def_win_proc();
}

//------------------------------------------------------------------------------
//! WM_CLOSE specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_CLOSE>(WPARAM, LPARAM) {
    if (on_close) {
        on_close();
    }

    return result_t::return_value(0);
}

namespace {
    //--------------------------------------------------------------------------
    //! Small helper to process raw input messages.
    //--------------------------------------------------------------------------
    struct raw_input_t {
        unsigned  code;      //<! input type
        HRAWINPUT handle;    //<! raw input handle
        UINT      size;      //<! size of the input data
        bool      has_extra; //<! extra information after the header

        //----------------------------------------------------------------------
        //! Get the size required for input messages.
        //! @throw platform::windows_exception
        //----------------------------------------------------------------------
        static UINT get_input_size(HRAWINPUT handle) {
            UINT size = 0;
            auto const result = ::GetRawInputData(
                handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER) );

            BK_THROW_ON_COND( ::GetRawInputData,
                static_cast<UINT>(result) == -1 );

            return size;
        }

        //----------------------------------------------------------------------
        //! Construct from the wParam and lParam received from WM_INPUT.
        //----------------------------------------------------------------------
        raw_input_t(WPARAM wParam, LPARAM lParam)
            : code(static_cast<uint8_t>(
                GET_RAWINPUT_CODE_WPARAM(wParam))
            ) //per docs
            , handle(reinterpret_cast<HRAWINPUT>(lParam))
            , size(get_input_size(handle))
            , has_extra(size > sizeof(RAWINPUT))
        {
        }

        //----------------------------------------------------------------------
        //! Get the information for input with no extra information.
        //! @throw platform::windows_exception
        //----------------------------------------------------------------------
        RAWINPUT get_simple() const {
            //! @todo handle non simple input data
            BK_ASSERT(!has_extra);

            RAWINPUT input;
            UINT     result_size = size;
            auto const result = ::GetRawInputData(
                handle, RID_INPUT, &input, &result_size, sizeof(RAWINPUTHEADER) );

            BK_THROW_ON_COND( ::GetRawInputData,
                static_cast<UINT>(result) == -1 );

            return input;
        }
    };

    //--------------------------------------------------------------------------
    //! Returns @c true if the button with index @c button went down,
    //! @c false otherwise.
    //--------------------------------------------------------------------------
    bool button_went_down(RAWMOUSE const& m, unsigned button) {
        return (m.usButtonFlags & (0x1 << (2*button))) != 0;
    }

    //--------------------------------------------------------------------------
    //! Returns @c true if the button with index @c button went up,
    //! @c false otherwise.
    //--------------------------------------------------------------------------
    bool button_went_up(RAWMOUSE const& m, unsigned button) {
        return (m.usButtonFlags & (0x2 << (2*button))) != 0;
    }

    class key_event_info {
    public:
        explicit key_event_info(RAWKEYBOARD const& kb) {
            went_down_ = !(kb.Flags & RI_KEY_BREAK);

            bool const is_e0    = (kb.Flags & RI_KEY_E0) != 0;
            bool const is_e1    = (kb.Flags & RI_KEY_E1) != 0;
            bool const is_pause = (kb.VKey == VK_PAUSE);
            
            // pause is a special case... bug in the API
            scancode_ = is_pause ? 0x45 : kb.MakeCode;

            // as per MapVirtualKeyExW docs and associated bug with VK_PAUSE
            UINT const flag = is_e0 ? (0xE0 << 8) :
                              is_e1 ? (0xE1 << 8) : 0;
        
            // virtual key code
            vkey_ = ::MapVirtualKeyExW(
                scancode_ | flag,
                MAPVK_VSC_TO_VK_EX,
                0
            );

            // set the extended bit
            if (is_e0) scancode_ |= 0x100;
        }

        std::wstring get_key_name() const {
            wchar_t name_buffer[16];
            auto const length = ::GetKeyNameTextW(
                (scancode_ << 16), // per docs
                name_buffer,
                BK_ARRAY_ELEMENT_COUNT(name_buffer)
            );

            BK_THROW_ON_COND(GetKeyNameTextW, length == 0);

            return std::wstring(name_buffer);
        }

        bool went_down() const { return went_down_; }
        UINT scancode() const { return scancode_; }
        UINT vkey() const { return vkey_; }
    private:
        UINT scancode_;
        UINT vkey_;
        bool went_down_;
    };
} // namespace

//------------------------------------------------------------------------------
//! WM_INPUT specialization.
//------------------------------------------------------------------------------
template <>
impl::window_impl::result_t
impl::window_impl::handle_message<WM_INPUT>(WPARAM wParam, LPARAM lParam) {
    static const unsigned BUTTON_COUNT = 5;

    raw_input_t input_info(wParam, lParam);
    auto const  data = input_info.get_simple();

    switch (data.header.dwType) {
    case RIM_TYPEKEYBOARD : {
        auto& k = data.data.keyboard;
        
        if (k.VKey == 0xFF) break; // discard escape sequences

        key_event_info info(k);

        if (info.went_down()) {
            if (on_key_down) on_key_down(info.vkey());
        } else {
            if (on_key_up)   on_key_up(info.vkey());
        }
    } break;
    case RIM_TYPEMOUSE : {
        auto& m = data.data.mouse;

        // scroll wheel.
        if ((m.usButtonFlags & RI_MOUSE_WHEEL) && on_mouse_scroll) {
            on_mouse_scroll(static_cast<SHORT>(m.usButtonData));
        }
        
        // mouse buttons.
        for (unsigned i = 0; m.usButtonFlags && (i < BUTTON_COUNT); ++i) {
            if (on_mouse_down && button_went_down(m, i)) on_mouse_down(i);
            if (on_mouse_up && button_went_up(m, i))     on_mouse_up(i);
        }

        // movement.
        if (on_mouse_move && (m.lLastX || m.lLastY)) {
            on_mouse_move(m.lLastX, m.lLastY);
        }
    } break;
    case RIM_TYPEHID : {
        ::DefRawInputProc((PRAWINPUT*)&data, 1, sizeof(RAWINPUTHEADER));
    } break;
    } // switch

     // must be called per docs.
    ::DefWindowProcW(handle_, WM_INPUT, wParam, lParam);

    return result_t::return_value(0);
}

//------------------------------------------------------------------------------
//! Called by the top level window proc. Dispatches messages to their
//! appropriate handler function.
//------------------------------------------------------------------------------
LRESULT impl::window_impl::window_proc_(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam
) {
    result_t result;

    //! Small convenience macro to declare case statements.
    #define BK_HANDLE_MESSAGE_CASE(MSG) \
        case MSG : result = handle_message<MSG>(wParam, lParam); break

    switch (msg) {
    BK_HANDLE_MESSAGE_CASE(WM_PAINT);
    BK_HANDLE_MESSAGE_CASE(WM_SIZE);
    BK_HANDLE_MESSAGE_CASE(WM_CLOSE);
    BK_HANDLE_MESSAGE_CASE(WM_INPUT);
    BK_HANDLE_MESSAGE_CASE(WM_MOUSEMOVE);
    BK_HANDLE_MESSAGE_CASE(WM_CHAR);
    }

    #undef BK_HANDLE_MESSAGE_CASE

    return result ?
        static_cast<LRESULT>(result) :
        ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

//------------------------------------------------------------------------------
//! Helper function to create a window.
//! @param window The window object which will own this HWND.
//------------------------------------------------------------------------------
HWND impl::window_impl::create_window_(
    window_impl* window // associated window object
) {
    auto const INSTANCE = ::GetModuleHandleW(L"");

    WNDCLASSEXW const wc = {
        sizeof(WNDCLASSEXW),
        CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
        window_impl::top_level_wnd_proc_,
        0,
        0,
        INSTANCE,
        ::LoadIconW(nullptr, MAKEINTRESOURCEW(IDI_WINLOGO)),
        ::LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW)),
        nullptr,
        nullptr,
        CLASS_NAME,
        nullptr
    };

    ::RegisterClassExW(&wc); // ignore return value

    auto const result = ::CreateWindowExW(
        0,
        CLASS_NAME,
        L"window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (HWND)nullptr,
        (HMENU)nullptr,
        INSTANCE,
        window
    );

    BK_THROW_ON_FAIL(::CreateWindowExW, result ? S_OK : E_FAIL);

    return result;
}

//------------------------------------------------------------------------------
//! Helper function to enable raw input for the window.
//------------------------------------------------------------------------------
void impl::window_impl::enable_raw_input() {
    RAWINPUTDEVICE const devices[] = {
        {0x01, 0x02, 0,               handle_},   // mouse
        {0x01, 0x06, RIDEV_NOHOTKEYS, handle_},   // keyboard
    };

    auto const result = ::RegisterRawInputDevices(
        devices,
        BK_ARRAY_ELEMENT_COUNT(devices),
        sizeof(BK_ARRAY_ELEMENT_TYPE(devices))
    );

    BK_THROW_ON_FAIL(::CreateWindowExW, result ? S_OK : E_FAIL);
}
