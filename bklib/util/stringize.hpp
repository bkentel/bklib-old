#pragma once

namespace bklib {
namespace util {

////////////////////////////////////////////////////////////////////////////////
//!
////////////////////////////////////////////////////////////////////////////////
struct stringized_base {
    typedef std::string                      string_t;   //! string type
    typedef std::unique_ptr<stringized_base> child_t;    //! sub-item type
    typedef std::vector<child_t>             children_t; //! container type

    //! returns a pointer to a container of sub-items; nullptr otherwise
    virtual children_t const* get_children() const = 0;

    stringized_base(
        string_t name, //! member or variable name
        string_t type, //! type as a string
        string_t value //! value as a string
    ) : name(std::move(name)), type(std::move(type)), value(std::move(value)) { }

    virtual ~stringized_base() { }

    string_t name, type, value;
};

typedef std::unique_ptr<stringized_base> stringized_t;

////////////////////////////////////////////////////////////////////////////////
//!
////////////////////////////////////////////////////////////////////////////////
struct stringized_simple : public stringized_base {
    //! simple => no sub-items
    children_t const* get_children() const override { return nullptr; }

    stringized_simple(string_t name, string_t type, string_t value)
        : stringized_base(std::move(name), std::move(type), std::move(value))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
//!
////////////////////////////////////////////////////////////////////////////////
struct stringized_composite : public stringized_base {
    //! get a container of sub-items
    children_t const* get_children() const override { return &children; }

    stringized_composite(
        string_t name,
        string_t type,
        string_t value = "struct", //! default value for a composite type
        size_t   count = 1         //! reserve size
    ) : stringized_base(std::move(name), std::move(type), std::move(value)) {
        this->children.reserve(count);
    }

    children_t children; //! sub-items
};

namespace detail {
    //! common base type
    struct type_string_helper_base {
        static unsigned const pointer_level = 0;
    };

    //! general base case template
    template <typename T>
    struct type_string_helper;

    //! recursive case template
    template <typename T>
    struct type_string_helper<T*> {
        static unsigned const pointer_level =
            1 + type_string_helper<T>::pointer_level;

        static char const* value() { return type_string_helper<T>::value(); }
    };

    ////////////////////////////////////////////////////////////////////////////
    #define BK_DEFINE_TYPE_STRING(TYPE)      \
        template <>                          \
        struct type_string_helper<TYPE>      \
            : public type_string_helper_base \
        {                                    \
            static char const* value() {     \
                return #TYPE;                \
            }                                \
        }
    //--------------------------------------------------------------------------
    BK_DEFINE_TYPE_STRING(char);
    BK_DEFINE_TYPE_STRING(bool);
    BK_DEFINE_TYPE_STRING(wchar_t);

    BK_DEFINE_TYPE_STRING(unsigned char);
    BK_DEFINE_TYPE_STRING(signed char);

    BK_DEFINE_TYPE_STRING(signed short);
    BK_DEFINE_TYPE_STRING(signed int);
    BK_DEFINE_TYPE_STRING(signed long);
    BK_DEFINE_TYPE_STRING(signed long long);

    BK_DEFINE_TYPE_STRING(unsigned short);
    BK_DEFINE_TYPE_STRING(unsigned int);
    BK_DEFINE_TYPE_STRING(unsigned long);
    BK_DEFINE_TYPE_STRING(unsigned long long);

    BK_DEFINE_TYPE_STRING(float);
    BK_DEFINE_TYPE_STRING(double);
    BK_DEFINE_TYPE_STRING(long double);
    //--------------------------------------------------------------------------
    #undef BK_DEFINE_TYPE_STRING
    ////////////////////////////////////////////////////////////////////////////

    //! get the typename for T
    template <typename T>
    std::string type_string(T&&) {
        typedef type_string_helper<
            typename std::remove_cv<
                typename std::remove_reference<T>::type
            >::type
        > helper_t;

        auto const value  = helper_t::value();
        auto const length = std::strlen(value); //include null terminator?? TODO
        auto const ptrs   = helper_t::pointer_level;

        std::string result;
        result.reserve(length + ptrs);

        result.assign(value, value + length);

        for (size_t i = 0; i < ptrs; ++i) {
            result.push_back('*');
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////
    #define BK_DEFINE_TO_DEBUG_BASIC_TYPE(TYPE)            \
        inline stringized_t             \
        stringize_fundamental(char const* name, TYPE const& var) {     \
            std::stringstream str; str << var;             \
            return std::unique_ptr<util::stringized_base>( \
                new util::stringized_simple(               \
                    name,                                  \
                    util::detail::type_string(var),        \
                    str.str()                              \
                )                                          \
            );                                             \
        }
    //--------------------------------------------------------------------------
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(char);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(bool);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(wchar_t);

    BK_DEFINE_TO_DEBUG_BASIC_TYPE(unsigned char);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(signed char);

    BK_DEFINE_TO_DEBUG_BASIC_TYPE(signed short);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(signed int);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(signed long);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(signed long long);

    BK_DEFINE_TO_DEBUG_BASIC_TYPE(unsigned short);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(unsigned int);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(unsigned long);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(unsigned long long);

    BK_DEFINE_TO_DEBUG_BASIC_TYPE(float);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(double);
    BK_DEFINE_TO_DEBUG_BASIC_TYPE(long double);
    //--------------------------------------------------------------------------
    #undef BK_DEFINE_TO_DEBUG_BASIC_TYPE
    ////////////////////////////////////////////////////////////////////////////

    //! specialization for C strings
    template <bool follow_pointers>
    stringized_t stringize_helper(char const* name, char* str) {
        return stringized_t(
            new stringized_simple(name, type_string(str), str)
        );
    }

    //! specialization for pointer types
    template <bool follow_pointers, typename T>
    stringized_t stringize_helper(char const* name, T const& var,
        //enable for pointer types
        typename std::enable_if<
            std::is_pointer<
                typename std::remove_reference<T>::type
            >::value
        >::type* = nullptr
    ) {
        std::stringstream value;
        value << "pointer = 0x" << reinterpret_cast<void const*>(var);

        if (follow_pointers && var) {
            auto result = std::unique_ptr<debug_string_parent>(
                new stringized_composite(name, type_string(var), value.str())
            );

            auto child = stringize_helper<true>(name, *var);
            result->children.emplace_back(std::move(child));

            return stringized_t(result.release());
        }

        return stringized_t(
            new stringized_simple(name, type_string(x), value.str())
        );
    }

    //! specialization for non-pointer types
    template <bool follow_pointers, typename T>
    stringized_t stringize_helper(char const* name, T const& x,
        //non-pointer types
        typename std::enable_if<
            !std::is_pointer<
                typename std::remove_reference<T>::type
            >::value
        >::type* = nullptr
    ) {
        return stringize_fundamental(name, x);
    }
} //namespace detail

//! specialization for pointer types
template <typename T>
stringized_t stringize(char const* name, T const& x, bool follow_pointers = false,
    //non-pointer types
    typename std::enable_if<
        !std::is_pointer<
            typename std::remove_reference<T>::type
        >::value
    >::type* = nullptr
) {
    (void)follow_pointers;
    return detail::stringize_helper<false>(name, x);
}

//! specialization for non-pointer types
template <typename T>
stringized_t stringize(char const* name, T const& x, bool follow_pointers = false,
    //pointer types
    typename std::enable_if<
        std::is_pointer<
            typename std::remove_reference<T>::type
        >::value
    >::type* = nullptr
) {
    return follow_pointers ?
        detail::stringize_helper<true>(name, x) :
        detail::stringize_helper<false>(name, x);
}

////////////////////////////////////////////////////////////////////////////////
#define BK_STRINGIZE_DEFINE_STRUCT_BEGIN(TYPE)                                 \
    ::bklib::util::stringized_t                                                \
    stringize_fundamental(char const* name, TYPE const& var) {                 \
        std::unique_ptr<::bklib::util::stringized_composite> result(           \
            new ::bklib::util::stringized_composite(name, #TYPE)               \
        );

#define BK_STRINGIZE_DEFINE_STRUCT_MEMBER(MEMBER)                              \
    result->children.push_back(                                                \
        ::bklib::util::stringize(#MEMBER, var.MEMBER, true)                    \
    );

#define BK_STRINGIZE_DEFINE_STRUCT_END                                         \
        return ::bklib::util::stringized_t(result.release());                  \
    }
////////////////////////////////////////////////////////////////////////////////
#define BK_STRINGIZE_DEFINE_ENUM_BEGIN(ENUM)                                   \
    ::bklib::util::stringized_t                                                \
    stringize_fundamental(char const* name, ENUM const& var) {                 \
        return ::bklib::util::stringized_t(                                    \
            new ::bklib::util::stringized_simple(name, #ENUM,

#define BK_STRINGIZE_DEFINE_ENUM_VALUE(VALUE)                                  \
    var == VALUE ? #VALUE :

#define BK_STRINGIZE_DEFINE_ENUM_END                                           \
    "UNKNOWN ENUM")); }
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! Flatten the stringized data depth first.
////////////////////////////////////////////////////////////////////////////////
std::ostream& flatten_stringized(std::ostream& out, stringized_t const& s);

} //namespace util
} //namespace bklib
