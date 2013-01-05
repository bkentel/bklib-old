////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Defines the base class for all exceptions thrown by the library.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <exception>
#include <boost/exception/all.hpp>

#include "types.hpp"

namespace bklib {

//==============================================================================
//! Base for all exceptions.
//==============================================================================
struct exception_base
    : virtual std::exception
    , virtual boost::exception
{
};

namespace detail {
    struct tag_error_message;
}

//==============================================================================
//! utf-8 string describing the error.
//==============================================================================
typedef boost::error_info<detail::tag_error_message, utf8string> error_message;

} //namespace bklib
