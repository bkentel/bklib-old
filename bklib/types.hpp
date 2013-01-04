#pragma once

#include <cvt/wstring>
#include <cstdint>
#include <string>
#include <codecvt>

namespace bklib {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

typedef std::string  utf8string;
typedef std::wstring platform_string;

typedef uint32_t utf32codepoint;

typedef std::wstring_convert<
    std::codecvt_utf8_utf16<wchar_t>
> utf8_16_converter;

} //namespace bklib
