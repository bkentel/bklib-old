#pragma once

#include <exception>
#include <boost/exception/all.hpp>

#include "types.hpp"

namespace bklib {

struct exception_base
    : virtual std::exception
    , virtual boost::exception
{
};

namespace detail {
    struct tag_error_message;
}

typedef boost::error_info<detail::tag_error_message, utf8string> error_message;

} //namespace bklib
