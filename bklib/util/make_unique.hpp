#pragma once

#include <type_traits>
#include <memory>

namespace std {

/*
//Broken!!
template <typename T, typename... Args>
inline unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}
*/

template <typename T>
unique_ptr<T> make_unique() {
    return unique_ptr<T>(new T());
}

template <typename T, typename P0>
unique_ptr<T> make_unique(P0&& p0) {
    return unique_ptr<T>(new T(forward<P0>(p0)));
}

template <typename T, typename P0, typename P1>
unique_ptr<T> make_unique(P0&& p0, P1&& p1) {
    return unique_ptr<T>(new T(forward<P0>(p0), forward<P1>(p1)));
}

template <typename T, typename P0, typename P1, typename P2>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2) {
    return unique_ptr<T>(new T(forward<P0>(p0), forward<P1>(p1), forward<P2>(p2)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2, P3&& p3) {
    return unique_ptr<T>(new T(forward<P0>(p0), forward<P1>(p1), forward<P2>(p2), forward<P3>(p3)));
}

} //namespace std
