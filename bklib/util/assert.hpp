#pragma once

namespace bklib { namespace detail {

struct assert_info_t_ {
    inline assert_info_t_(
        wchar_t const* function,
        wchar_t const* file,
        unsigned       line
    )
        : function(function), file(file), line(line)
    {
    }

   wchar_t const* function;
   wchar_t const* file;
   unsigned       line;
};

#define BK_MAKE_ASSERT_ ::bklib::detail::assert_info_t_(__FUNCTIONW__, __FILEW__, __LINE__)

template <typename... Args>
inline void assert_impl_(
    assert_info_t_&& info,
    char const*      condition
) {
}

template <typename... Args>
inline void assert_msg_impl_(
    assert_info_t_&& info,
    char const*      condition,
    char const*      fmt,
    Args&&...        args
) {
}

#define BK_ASSERT(condition)                                \
do {                                                        \
    if (!(condition)) {                                     \
        BK_BREAK;                                           \
        ::bklib::detail::assert_impl_(                      \
            BK_MAKE_ASSERT_, #condition);                   \
    }                                                       \
} while (false)

#define BK_ASSERT_MSG(condition, fmt, ...)                  \
do {                                                        \
    if (!(condition)) {                                     \
        BK_BREAK;                                           \
        ::bklib::detail::assert_msg_impl_(                  \
            BK_MAKE_ASSERT_, #condition, fmt, __VA_ARGS__); \
    }                                                       \
} while (false)

} //namespace detail
} //namespace bklib
