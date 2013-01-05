#pragma once

#include <Windows.h>
#include <combaseapi.h>

#include <d2d1.h>
#include <dwrite.h>

#include <Msctf.h>
#include <OleCtl.h>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include "exception.hpp"

namespace bklib {
namespace platform {

struct platform_exception : virtual exception_base {};

/////////////////////
struct windows_exception : virtual platform_exception {};

namespace detail  {
    struct tag_windows_error_code;
    struct tag_windows_hresult;
}

typedef boost::error_info<detail::tag_windows_error_code, DWORD>   windows_error_code;
typedef boost::error_info<detail::tag_windows_hresult,    HRESULT> windows_hresult;
/////////////////////
struct com_exception : virtual windows_exception {};

namespace detail  {
    struct tag_com_result_code;
}

typedef boost::error_info<detail::tag_com_result_code, HRESULT> com_result_code;

//==============================================================================
//! Helper to delete handles for use with std::unique_ptr.
//==============================================================================
template <typename T> struct handle_deleter;

//! HWND deleter.
template <> struct handle_deleter<HWND> {
    typedef HWND pointer;

    void operator()(HWND handle) const {
        ::DestroyWindow(handle);
    }
};

//! HGLRC deleter.
template <> struct handle_deleter<HGLRC> {
    typedef HGLRC pointer;

    void operator()(HGLRC handle) const {
        ::wglDeleteContext(handle);
    }
};

//! ATOM deleter.
template <> struct handle_deleter<ATOM> {
    typedef ATOM pointer;

    void operator()(ATOM id) const {
        ::UnregisterClassW(MAKEINTATOM(id), ::GetModuleHandleW(nullptr));
    }
};

namespace detail {
    template <typename F>
    struct handle_deleter_helper {
        typedef decltype(std::declval<F>()())        value_type;
        typedef handle_deleter<value_type>           deleter;
        typedef std::unique_ptr<value_type, deleter> type;
    };
} //namespace detail 

//==============================================================================
//! Helper to delete handles for use with std::unique_ptr.
//==============================================================================
template <typename F>
typename detail::handle_deleter_helper<F>::type make_unique_handle(F function) {
    return typename detail::handle_deleter_helper<F>::type(
        function()
    );
}

typedef std::unique_ptr<HWND,  handle_deleter<HWND>>  unique_hwnd;
typedef std::unique_ptr<HGLRC, handle_deleter<HGLRC>> unique_hglrc;

} //namespace platform
} //namespace bklib

//------------------------------------------------------------------------------
//! Macro to throw an exception when an API function fails
//------------------------------------------------------------------------------
#define BK_THROW_ON_FAIL(api, hr)                                              \
while (FAILED(hr)) {                                                           \
    BOOST_THROW_EXCEPTION(                                                     \
        ::bklib::platform::windows_exception()                                 \
        << ::boost::errinfo_api_function(#api)                                 \
        << ::bklib::platform::windows_hresult(hr)                              \
        << ::bklib::platform::windows_error_code(                              \
            [] {return ::GetLastError();}()                                    \
        )                                                                      \
    );                                                                         \
                                                                               \
    break;                                                                     \
} //----------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! Macro to throw an exception when an API function fails. That is, for
//! test = true
//------------------------------------------------------------------------------
#define BK_THROW_ON_COND(api, test)                                            \
while (test) {                                                                 \
    BOOST_THROW_EXCEPTION(                                                     \
        ::bklib::platform::windows_exception()                                 \
        << ::boost::errinfo_api_function(#api)                                 \
        << ::bklib::platform::windows_error_code(                              \
            [] {return ::GetLastError();}()                                    \
        )                                                                      \
    );                                                                         \
                                                                               \
    break;                                                                     \
} //----------------------------------------------------------------------------