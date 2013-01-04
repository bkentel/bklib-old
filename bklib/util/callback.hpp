//------------------------------------------------------------------------------
//! @file
//! @author Brandon Kentel
//! @date   Dec 2012
//! @brief  Macros to aid in defining callback functions.
//------------------------------------------------------------------------------
#pragma once

namespace bklib { namespace util {

//------------------------------------------------------------------------------
//! Describes the function type used for a callback.
//------------------------------------------------------------------------------
template <typename tag_t, typename sig_t>
struct handler_t {
    typedef std::function<sig_t> type;
};

//------------------------------------------------------------------------------
//! Declares the callback registration function @c listen(handler).
//! @note Must be within a class definition.
//------------------------------------------------------------------------------
#define BK_UTIL_CALLBACK_BEGIN             \
    template <typename T>                  \
    void listen(typename T::type handler)

//------------------------------------------------------------------------------
//! Declares a @c typedef to use for callbacks which has the form: event_<name>.
//! @note Must be within a class definition.
//------------------------------------------------------------------------------
#define BK_UTIL_CALLBACK_DECLARE(NAME, SIG) \
    typedef ::bklib::util::handler_t<struct tag_##NAME, SIG> event_##NAME

//------------------------------------------------------------------------------
//! Only used to visually end a block.
//! @note Must be within a class definition.
//------------------------------------------------------------------------------
#define BK_UTIL_CALLBACK_END static_assert(true, "")

//------------------------------------------------------------------------------
//! @breif Declare the @c listen(handler) specialization for @c event_<name> as
//!        being extern.
//! @note  Must be @e outside a class definition.
//------------------------------------------------------------------------------
#define BK_UTIL_CALLBACK_DECLARE_EXTERN(TYPE, NAME) \
    extern template void TYPE::listen<TYPE::event_##NAME>(TYPE::event_##NAME::type)

//------------------------------------------------------------------------------
//! Define the body of thr <type>::listen(event_<name>) specialization.
//! @note Must be @e outside a class definition.
//------------------------------------------------------------------------------
#define BK_UTIL_CALLBACK_DEFINE_IMPL(TYPE, NAME) \
    template <> void TYPE::listen<TYPE::event_##NAME>(event_##NAME::type handler)

} //namespace util
} //namespace bklib
