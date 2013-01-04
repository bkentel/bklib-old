#pragma once

#include <type_traits>

#define BK_UNUSED_VAR(x) (void)x
#define BK_BREAK __debugbreak()

#define BK_CONCAT_IMPL(a, b) a##b
#define BK_CONCAT(a, b) BK_CONCAT_IMPL(a, b)

namespace bklib { namespace detail {

template <typename T>
struct array_info;

template <typename T, size_t N>
struct array_info<T [N]> {
    typedef T type;
    static size_t const size = N;
};

} //namesapce detail
} //namesapce bklib

#define BK_ARRAY_ELEMENT_TRAITS(x)                          \
::bklib::detail::array_info<                                \
    typename std::remove_pointer<                           \
        typename std::remove_reference<decltype(x)>::type   \
    >::type                                                 \
>

#define BK_ARRAY_ELEMENT_COUNT(x) BK_ARRAY_ELEMENT_TRAITS(x)::size
#define BK_ARRAY_ELEMENT_TYPE(x)  BK_ARRAY_ELEMENT_TRAITS(x)::type

#define BK_TODO_BREAK __debugbreak(); std::terminate();
#define BK_TODO_MSG(msg) OutputDebugStringA(#msg)
