////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Assertions.
////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace bklib { namespace detail {
//==============================================================================
//! Holds source code information for assert.
//==============================================================================
struct assert_info {
    inline assert_info(
        char    const* function,
        wchar_t const* file,
        unsigned       line
    )
        : function(function), file(file), line(line)
    {
    }

   char    const* function;
   wchar_t const* file;
   unsigned       line;
};

//==============================================================================
//! Construct an assert_info object.
//==============================================================================
#define BK_ASSERT_HELPER \
    ::bklib::detail::assert_info(__FUNCTION__, __FILEW__, __LINE__)

//==============================================================================
//! Implementation for assert().
//==============================================================================
template <typename... Args>
inline void assert_impl(
    assert_info&& info,
    char const*   condition
) {
}

//==============================================================================
//! Implementation for assert(msg).
//==============================================================================
template <typename... Args>
inline void assert_msg_impl(
    assert_info&& info,
    char const*   condition,
    char const*   fmt,
    Args&&...     args
) {
}

//==============================================================================
//! Trigger a breakpoint when @c condition evaluates to @c false.
//==============================================================================
#define BK_ASSERT(condition)                                                   \
do {                                                                           \
    if (!(condition)) {                                                        \
        BK_BREAK;                                                              \
        ::bklib::detail::assert_impl(                                          \
            BK_ASSERT_HELPER, #condition);                                     \
    }                                                                          \
} while (false)

//==============================================================================
//! Trigger a breakpoint when @c condition evaluates to @c false and print a
//! message printf style.
//==============================================================================
#define BK_ASSERT_MSG(condition, fmt, ...)                                     \
do {                                                                           \
    if (!(condition)) {                                                        \
        BK_BREAK;                                                              \
        ::bklib::detail::assert_msg_impl(                                      \
            BK_ASSERT_HELPER, #condition, fmt, __VA_ARGS__);                   \
    }                                                                          \
} while (false)

} //namespace detail
} //namespace bklib
