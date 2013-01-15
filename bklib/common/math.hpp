////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Defines math and geometry related functions.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cmath>
#include <algorithm>
#include <functional>

#include "util/assert.hpp"

namespace bklib { namespace math {
////////////////////////////////////////////////////////////////////////////////
// Non-member functions
////////////////////////////////////////////////////////////////////////////////
template <typename T>
auto width(T const& x) -> decltype(x.width()) {
    return x.width();
}

template <typename T>
auto height(T const& x) -> decltype(x.height()) {
    return x.height();
}

template <typename T>
auto x(T const& v) -> decltype(v.x()) {
    return v.x();
}

template <typename T>
auto y(T const& v) -> decltype(v.y()) {
    return v.y();
}

template <typename T>
auto z(T const& v) -> decltype(v.z()) {
    return v.z();
}

template <typename T, typename U>
auto distance2(T const& a, U const& b) -> decltype(x(a) - x(b)) {
    auto const X = x(a) - x(b);
    auto const Y = y(a) - y(b);
    auto const Z = z(a) - z(b);

    return X*X + Y*Y + Z*Z;
}

template <typename T, typename U>
auto distance(T const& a, U const& b) -> decltype(std::sqrt(distance2(a, b))) {
    return std::sqrt(distance2(a, b));
}

//------------------------------------------------------------------------------
//! Point.
//! @t-param T
//!     Scalar type.
//! @t-param N
//!     Dimensions.
//------------------------------------------------------------------------------
template <typename T, size_t N>
struct point {
    static size_t const dimension = N;

    template <size_t M, typename... Args>
    void assign(T arg, Args... args) {
        static_assert(M < N, "wrong number of arguments");
        p_[M] = arg;
        assign<M+1>(args...);
    }

    template <size_t M>
    void assign(T arg) {
        static_assert(M < N, "wrong number of arguments");
        p_[M] = arg;
    }

    template <typename... Args>
    point(T arg, Args... args) {
        static_assert(sizeof...(Args) + 1 == N, "wrong number of arguments");
        assign<0>(arg, args...);
    }
    
    template <size_t M, typename U>
    void assign(point<U, N> const& p) {
        p_[M] = static_cast<T>(p.p_[M]);
    }

    template <typename U>
    explicit point(point<U, N> const& p) {
        assign<0>(p);
    }
    
    point(std::initializer_list<T> list) {
        std::copy_n(list.begin(), N, p_);
    }

    template <size_t M>
    T get() const {
        static_assert(M < N, "index out of range");
        return p_[M];
    }

    template <size_t M>
    T& get() {
        static_assert(M < N, "index out of range");
        return p_[M];
    }

    template <size_t M>
    static bool equal(point const& a, point const &b) {
        return (a.get<M> == b.get<M>) && equal<M-1>(a, b);
    }

    template <>
    static bool equal<0>(point const&, point const&) {
        return true;
    }

    bool operator==(point const& rhs) const {
        return equal<N>(*this, rhs);
    }

    T p_[N];
};

template <typename T, size_t N>
T width(point<T, N> const& x) {  return static_cast<T>(0); }

template <typename T, size_t N>
T height(point<T, N> const& x) { return static_cast<T>(0); }

template <typename T, size_t N>
T x(point<T, N> const& v) { return v.get<0>(); }

template <typename T, size_t N>
T y(point<T, N> const& v) { return v.get<1>(); }

template <typename T, size_t N>
T z(point<T, N> const& v) { return v.get<2>(); }

//------------------------------------------------------------------------------
//! Numerical range.
//! @t-param T
//!     Scalar type.
//! @t-param lower_t
//!     Comparison operator used for the lower bound; default is >=.
//! @t-param upper_t
//!     Comparison operator used for the upper bound; default is <=.
//------------------------------------------------------------------------------
template <
    typename T,
    typename lower_t = std::greater_equal<T>,
    typename upper_t = std::less_equal<T>
>
struct range {
    range(T min, T max)
        : min(min), max(max)
    {
    }

    T clamp(T x) const {
        return !lower_t()(x, min) ? min :
               !upper_t()(x, max) ? max : x;
    }

    T min, max;
};

//------------------------------------------------------------------------------
//! Common empty base class for rectangles.
//------------------------------------------------------------------------------
struct rect_base {
    enum class side_x {
        none  =  0,
        left  = -1,
        right =  1
    };

    enum class side_y {
        none   =  0,
        top    = -1,
        bottom =  1
    };

    template <typename T>
    static T side_sign(side_x side) {
        return static_cast<T>(side);
    }

    template <typename T>
    static T side_sign(side_y side) {
        return static_cast<T>(side);
    }
};

//------------------------------------------------------------------------------
//! Rectangle.
//! @t-param T
//!     Scalar type.
//------------------------------------------------------------------------------
template <typename T>
struct rect : public rect_base {
    typedef T value_type;

    void resize(side_x side, T dx) {        
        get_side(side) += side_sign<T>(side) * dx;
        check();
    }

    void resize(side_y side, T dy) {        
        get_side(side) += side_sign<T>(side) * dy;
        check();
    }

    void translate_x(T dx) {
        left  += dx;
        right += dx;
    }

    void translate_y(T dy) {
        bottom += dy;
        top    += dy;
    }

    void translate(T dx, T dy) {
        translate_x(dx);
        translate_y(dy);
    }

    //! @todo allow movement from any side.
    void move_to(T x, T y) {
        translate(x - left, y - top);
    }

    rect(T left, T top, T right, T bottom)
        : left(left), top(top), right(right), bottom(bottom)
    {
        check();
    }

    template <typename U>
    rect(U const& v, T width, T height)
        : left(x(v))
        , top(y(v))
        , right(left + width)
        , bottom(top + height)
    {
    }

    T width() const {
        return right - left;
    }

    T height() const {
        return bottom - top;
    }

    //--------------------------------------------------------------------------
    T& get_side(side_x side) {
        return side == side_x::right ? right : left;
    }

    T& get_side(side_y side) {
        return side == side_y::bottom ? bottom : top;
    }

    T const& get_side(side_x side) const {
        return side == side_x::right ? right : left;
    }

    T const& get_side(side_y side) const {
        return side == side_y::bottom ? bottom : top;
    }

    //! not inlined by default by MSVC 2012
    __forceinline bool is_degenerate() const {
        return (left > right) || (top > bottom);
    }

    void check() const {
        BK_ASSERT_MSG(!is_degenerate(), "degenerate rect");
    }
    //--------------------------------------------------------------------------
    T magnitude(side_x) const { return width();  }
    T magnitude(side_y) const { return height(); }

    template <typename S>
    T resize_constrained(S side, T delta, range<T> const& constraint) {
        auto const old_dist = magnitude(side);
        auto const new_dist = old_dist + side_sign<T>(side) * delta;
        auto const clamped  = constraint.clamp(new_dist);
        auto const change   = clamped - old_dist;
        
        get_side(side) += change * side_sign<T>(side);
        check();

        return change;
    }

    T left, top, right, bottom;
};

template <typename T, rect_base::side_x Side = rect_base::side_x::left>
T x(rect<T> const& r) { return r.get_side(Side); }

template <typename T, rect_base::side_y Side = rect_base::side_y::top>
T y(rect<T> const& r) { return r.get_side(Side); }

//------------------------------------------------------------------------------
//! Rectangle constrained by a maximum and minimum height and width.
//------------------------------------------------------------------------------
//template <typename T>
//struct constrained_rect : public rect<T> {
//    typedef rect<T> rect_t;
//
//    constrained_rect(rect_t const& r,
//        T min_width,  T max_width,
//        T min_height, T max_height
//    )
//        : rect(r)
//        , constraint_w(min_width, max_width)
//        , constraint_h(min_height, max_height)
//    {
//        check();
//    }
//
//    range<T>& get_constraint(side_x) { return constraint_w; }
//    range<T>& get_constraint(side_y) { return constraint_h; }
//
//    T distance(side_x) const { return width();  }
//    T distance(side_y) const { return height(); }
//
//    template <typename U>
//    T resize(U side, T delta) {
//        auto const old_dist = distance(side);
//        auto const new_dist = old_dist + side_sign<T>(side) * delta;
//        auto const clamped  = get_constraint(side).clamp(new_dist);
//        auto const change   = clamped - old_dist;
//        
//        get_side(side) += change;
//        check();
//
//        return change;
//    }
//
//    range<T> constraint_w;
//    range<T> constraint_h;
//};

//------------------------------------------------------------------------------
//! intersection of the point (x, y) with the rectangle @c rect.
//! @return
//!     @c true if there is an intersection.
//!     @c false otherwise.
//------------------------------------------------------------------------------
template <typename T>
bool intersects(T x, T y, rect<T> const& rect) {
    return !( x < rect.left || x > rect.right  ||
              y < rect.top  || y > rect.bottom );
}

//------------------------------------------------------------------------------
//! intersection of the point @p with the rectangle @c rect.
//! @return
//!     @c true if there is an intersection.
//!     @c false otherwise.
//------------------------------------------------------------------------------
template <typename T>
bool intersects(point<T, 2> const& p, rect<T> const& rect) {
    auto const result = intersects(x(p), y(p), rect);
    return result;
}

//------------------------------------------------------------------------------
//! Describes the intersection(s) (if any) with a point and the border of a
//! rectangle.
//------------------------------------------------------------------------------
struct border_intersection {
    bool is_inside;
    rect_base::side_x x;
    rect_base::side_y y;

    border_intersection()
        : is_inside(false)
        , x(rect_base::side_x::none)
        , y(rect_base::side_y::none)
    {
    }

    //! @c true if there is at least one intersection.
    explicit operator bool() const {
        return (x != rect_base::side_x::none) ||
               (y != rect_base::side_y::none);
    }
};

//------------------------------------------------------------------------------
//! Intersection of the point (x, y) with the border of thickness @c border_size
//! of rectangle @c rect.
//! @return A border_intersection pbject describing the intersection(s) if any.
//------------------------------------------------------------------------------
template <typename T>
border_intersection
intersects_border(T x, T y, rect<T> const& rect, T border_size) {
    border_intersection result;

    bool const is_left = x >= rect.left && x <= (rect.left + border_size);
    bool const is_top  = y >= rect.top  && y <= (rect.top  + border_size);

    if (is_left) {
        result.x = rect_base::side_x::left;
    } else {
        bool const is_right = x <= rect.right && x >= (rect.right - border_size);
        result.x = is_right ? rect_base::side_x::right : rect_base::side_x::none;
    }

    if (is_top) {
        result.y = rect_base::side_y::top;
    } else {
        bool const is_bottom = y <= rect.bottom && y >= (rect.bottom - border_size);
        result.y = is_bottom ? rect_base::side_y::bottom : rect_base::side_y::none;
    }

    result.is_inside = intersects(x, y, rect);

    return result;
}

//------------------------------------------------------------------------------
//! Intersection of the point @c p with the border of thickness @c border_size
//! of rectangle @c rect.
//! @return A border_intersection pbject describing the intersection(s) if any.
//------------------------------------------------------------------------------
template <typename T>
border_intersection
intersects_border(point<T, 2> const& p, rect<T> const& rect, T border_size) {
    return intersects_border(x(p), y(p), rect, border_size);
}


} //namespace math
} //namespace bklib
