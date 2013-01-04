////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Unclassified utilities. 
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <type_traits>

#include "macros.hpp"

namespace bklib {


template <typename T, typename... Args>
void safe_callback(T&& callback, Args&&... args) {
    if (callback) {
        callback(std::forward<Args>(args)...);
    }
}

template <typename T>
struct reverse_adapter_t {
    explicit reverse_adapter_t(T& c) : c(std::addressof(c)) { }

    typename T::const_reverse_iterator begin() const {
        return c->rbegin();
    }

    typename T::const_reverse_iterator end() const {
        return c->rend();
    }

    typename T::const_reverse_iterator cbegin() const {
        return c->crbegin();
    }

    typename T::const_reverse_iterator cend() const {
        return c->crend();
    }

    T* c;
};

template <typename T>
reverse_adapter_t<T> reverse_adapter(T& c) {
    return reverse_adapter_t<T>(c);
}

template <typename F>
struct remove_class;

template <typename R, typename C, typename P1, typename P2, typename P3>
struct remove_class<R (C::*)(P1, P2, P3)> {
    typedef int type;
};

template <typename T>
auto remove_const(T x) -> typename std::remove_const<T>::type {
    return const_cast<typename std::remove_const<T>::type>(x);
}

//template <typename C, typename F>
//void for_all(C&& container, F&& function) {
//    std::for_each(
//        std::begin(container),
//        std::end(container),
//        [&function](decltype(*std::begin(container))& i) {
//            function(i);
//        }
//    );
//}

template <typename T>
struct base_type {
    typedef typename std::remove_cv<
        typename std::remove_all_extents<T>::type
    >::type type;
};

template <typename T>
struct base_type<T*> {
    typedef typename base_type<T>::type type;
};

} //namespace bklib
