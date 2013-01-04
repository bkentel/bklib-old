////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Expected value (expected value, otherwise stored exception_ptr).
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <new>

#include "traits.hpp"

namespace bklib {

//! compile time max(A, B)
template <unsigned A, unsigned B>
struct max_of {
    static unsigned const value = B < A ? A : B;
};

//! Decide at compile time whether to rethrow the active exception
template <bool B> static inline void rethrow_exception();
template <> __declspec(noreturn) static inline void rethrow_exception<true>()  { throw; }
template <> static inline void rethrow_exception<false>() { }

////////////////////////////////////////////////////////////////////////////////
//! Contains a T or an exception derived from std::exception describing the
//! reason a T failed to be created.
////////////////////////////////////////////////////////////////////////////////
template <typename T>
class expected {
public:
    ////////////////////////////////////////////////////////////////////////////
    //! Explicitly destroy T or the exception_ptr
    ////////////////////////////////////////////////////////////////////////////
    ~expected() {
        if (is_valid()) {
            as_value_().~T();
        } else {
            using std::exception_ptr; //only as a workaround for a parsing quirk
            as_exception_().~exception_ptr();
        }
    }

    expected() : is_expected_(true) {
        static_assert(std::is_default_constructible<T>::value,
            "T must be default constructible here");

        ::new(where_()) T();
    }

    ////////////////////////////////////////////////////////////////////////////
    //! Construct from another expected
    ////////////////////////////////////////////////////////////////////////////
    expected(expected const& other) : is_expected_(other.is_expected_) {
        if (other.is_valid()) {
            ::new(where_()) T(other.as_value_());
        } else {
            ::new(where_()) std::exception_ptr(other.as_exception_());
        }
    }

    expected(expected&& other) : is_expected_(other.is_expected_) {
        if (other.is_valid()) {
            ::new(where_()) T(std::move(other.as_value_()));
        } else {
            ::new(where_()) std::exception_ptr(std::move(other.as_exception_()));
        }
    }

    expected& operator=(expected const& rhs) {
        using std::swap;

        auto temp = rhs;
        swap(*this, temp);

        return *this;
    }

    expected& operator=(expected&& rhs) {
        swap(rhs);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////
    //! Construct from a T
    ////////////////////////////////////////////////////////////////////////////
    template <typename U>
    expected(U const& value,
        typename std::enable_if<
            std::is_base_of<T, U>::value
        >::type* = nullptr
    ) : is_expected_(true) {
        static_assert(std::is_same<U, T>::value, "will be sliced");
        ::new(where_()) T(value);
    }

    template <typename U>
    expected(U&& value,
        typename std::enable_if<
            std::is_base_of<T, U>::value
        >::type* = nullptr
    ) : is_expected_(true) {
        static_assert(std::is_same<U, T>::value, "will be sliced");
        ::new(where_()) T(std::move(value));
    }

    ////////////////////////////////////////////////////////////////////////////
    //! Construct from an exception derived from std::exception
    ////////////////////////////////////////////////////////////////////////////
    template <typename E, typename = typename std::enable_if<
        std::is_base_of<std::exception, E>::value>::type>
    expected(E const& exception) : is_expected_(false) {
        make_from_exception_(exception);
    }

    template <typename E, typename = typename std::enable_if<
        std::is_base_of<std::exception, E>::value>::type
    >
    expected(E&& exception) : is_expected_(false) {
        make_from_exception_(std::forward<E>(exception));
    }

    ////////////////////////////////////////////////////////////////////////////
    //! Construct from an exception_ptr
    ////////////////////////////////////////////////////////////////////////////
    expected(std::exception_ptr exception) : is_expected_(false) {
        ::new(where_()) std::exception_ptr(std::move(exception));
    }

    ////////////////////////////////////////////////////////////////////////////
    //! Construct from a nullary callable object
    ////////////////////////////////////////////////////////////////////////////
    template <typename F, typename = decltype(std::declval<F>()())>
    expected(F function) : is_expected_(true) {
        try {
            ::new(where_()) T(function());
        } catch(...) {
            is_expected_ = false;
            ::new(where_()) std::exception_ptr(std::current_exception());
        }
    }

    bool is_valid() const {
        return is_expected_;
    }

    T& get() {
        if (!is_valid()) std::rethrow_exception(as_exception_());
        return as_value_();
    }
    T const& get() const {
        if (!is_valid()) std::rethrow_exception(as_exception_());
        return as_value_();
    }

    //! The std::exception_ptr if the stored exception is an E, otherwise std::exception_ptr(nullptr)
    template <typename E = std::exception>
    std::exception_ptr get_exception() {
        static_assert(std::is_base_of<std::exception, E>::value,
            "E must be derived from std::exception.");

        if (!is_valid()) {
            try {
                std::rethrow_exception(as_exception_());
            } catch (E const&) {
                return as_exception_();
            } catch (...) {
            }
        }

        return std::exception_ptr(nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////
    //! If the stored exception_ptr is convertable to E, call function(E&), and
    //! for Rethrow = true, rethrow. Otherwise return true. If the exception_ptr
    //! is not converable to to E, return false.
    ////////////////////////////////////////////////////////////////////////////
    template <bool Rethrow = false, typename F>
    bool with_exception(F function) {
        typedef traits::callable_traits<F>::arg<0>::type
            exception_type;
        typedef typename std::remove_reference<exception_type>::type
            non_ref_exception_type;

        static_assert(std::is_reference<exception_type>::value,
            "function must take a reference");
        static_assert(
            std::is_base_of<
                std::exception,
                non_ref_exception_type
            >::value,
            "function must take a type derived from std::exception"
        );

        if (is_valid()) return false;

        try {
            std::rethrow_exception(as_exception_());
        } catch (exception_type e) {
            function(e);
            rethrow_exception<Rethrow>();
            return true;
        } catch (...) {
        }

        return false;
    }

    void swap(expected& rhs) {
        if (is_valid()) {
            //(1) good good
            if (rhs.is_valid()) {
                using std::swap;
                swap(as_value_(), rhs.as_value_());
            //(2) good bad
            } else {
                auto temp = std::move(rhs.as_exception_());

                ::new(rhs.where_()) T(std::move(as_value_()));
                ::new(where_())     std::exception_ptr(std::move(temp));

                std::swap(is_expected_, rhs.is_expected_);
            }
        } else {
            //(3) good bad
            if (rhs.is_valid()) {
                rhs.swap(*this); //complementary case to (2)
            //(4) bad bad
            } else {
                std::swap(as_exception_(),  rhs.as_exception_());
            }
        }
    }
private:
    //! workaround for the lack of C++11 unions in MSVC
    typedef typename std::aligned_storage<
        max_of<
            sizeof(T),
            sizeof(std::exception_ptr)
        >::value,
        max_of<
            std::alignment_of<T>::value,
            std::alignment_of<std::exception_ptr>::value
        >::value
    >::type storage_type_;

    //! workaround for the lack of C++11 unions in MSVC
    storage_type_ storage_;
    bool          is_expected_;

    void* where_() {
        return reinterpret_cast<void*>(&storage_);
    }

    //! workaround for the lack of C++11 unions in MSVC
    T& as_value_() {
        return *reinterpret_cast<T*>(&storage_);
    }
    //! workaround for the lack of C++11 unions in MSVC
    T const& as_value_() const {
        return *reinterpret_cast<T const*>(&storage_);
    }

    //! workaround for the lack of C++11 unions in MSVC
    std::exception_ptr& as_exception_() {
        return *reinterpret_cast<std::exception_ptr*>(&storage_);
    }
    //! workaround for the lack of C++11 unions in MSVC
    std::exception_ptr const& as_exception_() const {
        return *reinterpret_cast<std::exception_ptr const*>(&storage_);
    }

    //! helper function to construct from an exception
    template <typename E>
    void make_from_exception_(E&& exception) {
        //! the dynamic and static types must match or the object will have
        //! been sliced.
        if (typeid(E) != typeid(exception)) {
            assert(false && "sliced!");
        }

        ::new(where_()) std::exception_ptr(
            std::make_exception_ptr(
                std::forward<E>(exception)
            )
        );
    }
};

namespace detail {
    //! Workaround for a compiler bug in MSVC
    template <typename T, typename F>
    expected<T> make_expected_helper(F function) {
        try {
            return expected<T>(function());
        } catch (...) {
            return expected<T>(std::current_exception());
        }
    }
} //namespace detail

template <typename T, typename... Args>
expected<T> make_expected(Args&&... args) {
    //! workaround for a compiler bug in MSVC
    return detail::make_expected_helper<T>([&] {
        return T(std::forward<Args>(args)...);
    });
}

template <typename T>
void swap(expected<T>& lhs, expected<T>& rhs) {
    lhs.swap(rhs);
}

namespace unit_test {
    extern void test_expected();
} //namespace unit_test

} // namespace bklib
