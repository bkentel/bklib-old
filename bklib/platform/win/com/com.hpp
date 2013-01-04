#pragma once

//#include <combaseapi.h>
#include "util/util.hpp"
#include "util/traits.hpp"

namespace bklib {
namespace platform {
namespace win {

template <
    typename T
>
struct com_ptr {
    ~com_ptr() {
        release_();
    }

    com_ptr() : ptr_(nullptr) {
    }

    com_ptr(com_ptr const& other) : ptr_(other.ptr_) {
        add_ref_();
    }

    com_ptr(com_ptr&& other) : com_ptr() {
        std::swap(ptr_, other.ptr_);
    }

    explicit com_ptr(T* ptr) : ptr_(ptr) {
    }

    template <typename F, typename = decltype(std::declval<F>().operator())>
    explicit com_ptr(F&& function) : com_ptr() {
        //auto const hr = function(&ptr_); //! @todo
        function(&ptr_);
    }

    template <typename S>
    com_ptr(com_ptr<S>& other) : com_ptr() {
        static_assert(!std::is_same<T, S>::value, "invalid conversion construction");
        auto const hr = other.query_interface_(&ptr_);
    }

    com_ptr& operator=(com_ptr const& rhs) {
        if (rhs.ptr_) {
            rhs.ptr_->AddRef();
        }

        release_();
        ptr_ = rhs.ptr_;

        return *this;
    }

    com_ptr& operator=(com_ptr&& rhs) {
        std::swap(ptr_, rhs.ptr_);

        return *this;
    }

    template <typename S>
    com_ptr& operator=(com_ptr<S>& rhs) {
        static_assert(!std::is_same<T, S>::value, "invalid conversion assignment");

        T* ptr = nullptr;
        auto const hr = rhs.query_interface_(&ptr);
        ptr_ = ptr;

        return *this;
    }

    T* operator->() { return ptr_; }
    T const* operator->() const { return ptr_; }

    explicit operator bool() const { return ptr_ != nullptr; }

    operator T const*() const { return ptr_; }
    operator T*() { return ptr_; }

    template <typename T>
    HRESULT query_interface_(T* out) {
        if (ptr_) return ptr_->QueryInterface(&out);
        else      return E_FAIL;
    }

    void add_ref_() {
        if (ptr_) ptr_->AddRef();
    }

    void release_() {
        if (ptr_) ptr_->Release();
    }

    T* ptr_;
};

namespace detail {
    ////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct get_com_ptr_t {
        typedef bklib::traits::callable_traits<T> traits_t;

        typedef typename traits_t::result_t result_t_;
        static_assert(std::is_same<result_t_, HRESULT>::value, "must be return an HRESULT");

        typedef typename traits_t::arg<0>::type arg_t0_;
        static_assert(std::is_pointer<arg_t0_>::value, "must be a ** type");

        typedef typename std::remove_pointer<arg_t0_>::type arg_t1_;
        static_assert(std::is_pointer<arg_t0_>::value, "must be a ** type");

        typedef typename std::remove_pointer<arg_t1_>::type arg_t2_;
        static_assert(std::is_base_of<::IUnknown, arg_t2_>::value, "must be derived from IUnknown");

        typedef com_ptr<arg_t2_> type;
    };
} //namespace detail

////////////////////////////////////////////////////////////////////////////////
template <typename T>
auto make_com_ptr(T&& function) -> typename detail::get_com_ptr_t<
    typename std::remove_reference<T>::type
>::type
{
    typedef typename detail::get_com_ptr_t<
        typename std::remove_reference<T>::type
    >::type result_t;

    return result_t(std::forward<T>(function));
}

struct variant {
    variant() {
        VariantInit(&value);
    }

    ~variant() {
        VariantClear(&value);
    }

    operator VARIANT*() { return &value; }
    VARIANT* operator->() { return &value; }

    VARIANT value;
};

namespace detail {
    template <typename T>
    struct enum_result {
        enum_result(HRESULT result, T&& value)
            : valid_(SUCCEEDED(result) && result != S_FALSE)
            , value_(std::move(value))
        {
        }

        explicit operator bool() const {
            return valid_;
        }

        T& value() { return value_; }

        bool valid_;
        T    value_;
    };

    template <typename V, typename I = std::is_base_of<IUnknown, V>::type>
    struct get_enum_value;

    template <typename V>
    struct get_enum_value<V, std::true_type> {
        template <typename T>
        static enum_result<win::com_ptr<V>> get(T& enumerator) {
            V* out = nullptr;
            auto const result = enumerator->Next(1, &out, nullptr);
            return enum_result<win::com_ptr<V>>( result, win::com_ptr<V>(out) );
        }

        static V& pass(enum_result<win::com_ptr<V>>& x) {
            return *x.value();
        }
    };

    template <typename V>
    struct get_enum_value<V, std::false_type> {
        template <typename T>
        static enum_result<V> get(T& enumerator) {
            V out;
            auto const result = enumerator->Next(1, &out, nullptr);
            return enum_result<V>( result, std::move(out) );
        }

        static V& pass(enum_result<V>& x) {
            return x.value();
        }
    };
} // namespace detail

template <typename T, typename F>
void for_each_enum(T&& enumerator, F function) {
    typedef typename bklib::traits::callable_traits<F>::arg<0>::type value_t;
    typedef typename std::remove_reference<value_t>::type non_ref_value_t;
    typedef detail::get_enum_value<non_ref_value_t> enum_value;

    for (;;) {
        auto v = enum_value::get(enumerator);
        if (v) function(enum_value::pass(v));
        else return;
    }
}

} //namespace win
} //namespace platform
} //namespace bklib
