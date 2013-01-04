#pragma once

#include <Windows.h>
#include <combaseapi.h>

#include <d2d1.h>
#include <dwrite.h>

#include <Msctf.h>
#include <OleCtl.h>

#undef min
#undef max

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