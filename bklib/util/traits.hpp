#pragma once

#include <tuple>
#include <type_traits>

namespace bklib {
namespace traits {

////////////////////////////////////////////////////////////////////////////////
namespace detail {
    ////////////////////////////////////////////////////////////////////////////
    //! Select between function pointer types
    ////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct callable_helper_ptr;

    //! non-member functions
    template <typename R, typename... Args>
    struct callable_helper_ptr<R (*)(Args...)> {
        typedef void                object_t;
        typedef R                   result_t;
        typedef std::tuple<Args...> args_t;
    };

    //! member functions
    template <typename R, typename O, typename... Args>
    struct callable_helper_ptr<R (O::*)(Args...)> {
        typedef O                   object_t;
        typedef R                   result_t;
        typedef std::tuple<Args...> args_t;
    };

    //! const member functions
    template <typename R, typename O, typename... Args>
    struct callable_helper_ptr<R (O::*)(Args...) const> {
        typedef O                   object_t;
        typedef R                   result_t;
        typedef std::tuple<Args...> args_t;
    };

    ////////////////////////////////////////////////////////////////////////////
    //! Select between function pointers and functors
    ////////////////////////////////////////////////////////////////////////////
    template <typename T, typename is_ptr = typename std::is_pointer<T>::type>
    struct callable_helper;

    //! specialization for functors (and lambdas)
    template <typename T>
    struct callable_helper<T, std::false_type> {
        typedef callable_helper_ptr<decltype(&T::operator())> type;
    };

    //! specialization for function pointers
    template <typename T>
    struct callable_helper<T, std::true_type> {
        typedef callable_helper_ptr<T> type;
    };
} //namespace detail

////////////////////////////////////////////////////////////////////////////////
//! defines the various details of a callable object T
////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct callable_traits {
    typedef typename detail::callable_helper<T>::type::object_t object_t;
    typedef typename detail::callable_helper<T>::type::result_t result_t;
    typedef typename detail::callable_helper<T>::type::args_t   args_t;

    template <unsigned N>
    struct arg : public std::tuple_element<N, args_t> {};
};

} //namespace traits
} //namespace bklib
