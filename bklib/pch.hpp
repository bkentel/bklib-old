#pragma once

////////////////////////////////////////////////////////////////////////////////
// Utilities library
////////////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <csignal>
#include <csetjmp>
#include <cstdarg>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <bitset>
#include <functional>
#include <utility>
#include <ctime>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <tuple>
////////////////////////////////////////////////////////////////////////////////
// Dynamic memory management
////////////////////////////////////////////////////////////////////////////////
#include <new>
#include <memory>
#include <scoped_allocator>
////////////////////////////////////////////////////////////////////////////////
// Numeric limits
////////////////////////////////////////////////////////////////////////////////
#include <climits>
#include <cfloat>
#include <cstdint>
////////////////////////////////////////////////////////////////////////////////
// #include <cinttypes>
////////////////////////////////////////////////////////////////////////////////
#include <limits>
////////////////////////////////////////////////////////////////////////////////
// Error handling
////////////////////////////////////////////////////////////////////////////////
#include <exception>
#include <stdexcept>
#include <cassert>
#include <system_error>
#include <cerrno>
////////////////////////////////////////////////////////////////////////////////
// Strings library
////////////////////////////////////////////////////////////////////////////////
#include <cctype>
#include <cwctype>
#include <cstring>
//#include <cwstring>
#include <cwchar>
//#include <cuchar>
#include <string>
////////////////////////////////////////////////////////////////////////////////
// Containers library
////////////////////////////////////////////////////////////////////////////////
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
////////////////////////////////////////////////////////////////////////////////
// Algorithms library
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
////////////////////////////////////////////////////////////////////////////////
// Iterators library
////////////////////////////////////////////////////////////////////////////////
#include <iterator>
////////////////////////////////////////////////////////////////////////////////
// Numerics library
////////////////////////////////////////////////////////////////////////////////
#include <cmath>
#include <complex>
#include <valarray>
#include <random>
#include <numeric>
#include <ratio>
//#include <cfenv>
////////////////////////////////////////////////////////////////////////////////
// Input/output library
////////////////////////////////////////////////////////////////////////////////
#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>
#include <iomanip>
#include <streambuf>
#include <cstdio>
////////////////////////////////////////////////////////////////////////////////
// Localization library
////////////////////////////////////////////////////////////////////////////////
#include <locale>
#include <clocale>
#include <codecvt>
////////////////////////////////////////////////////////////////////////////////
// Regular Expressions library
////////////////////////////////////////////////////////////////////////////////
#include <regex>
////////////////////////////////////////////////////////////////////////////////
// Atomic Operations library
////////////////////////////////////////////////////////////////////////////////
#include <atomic>
////////////////////////////////////////////////////////////////////////////////
// Thread support library
////////////////////////////////////////////////////////////////////////////////
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

////////////////////////////////////////////////////////////////////////////////
// Boost libraries
////////////////////////////////////////////////////////////////////////////////
#include <boost/circular_buffer.hpp>
#include <boost/exception/all.hpp>

///////////////////////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////////////////////////
#include "config.hpp"

////////////////////////////////////////////////////////////////////////////////
// Platform specific includes
////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
#   include "platform/win/platform.hpp"
#endif

#include "util/macros.hpp"
#include "exception.hpp"
#include "util/make_unique.hpp"
#include "util/assert.hpp"
#include "types.hpp"
#include "util/scope_exit.hpp"
