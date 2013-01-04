#pragma once

#include <type_traits>
#include <cstdint>

namespace bklib {
namespace gfx {

template <uint8_t R, uint8_t G, uint8_t B, uint8_t A = 0xFF>
struct make_color_code {
    static uint32_t const value = static_cast<uint32_t>((R << 0x00) | (G << 0x08) | (B << 0x10) | (A << 0x18));
};

#define BK_COLOR_RGBA_FROM_FLOAT(r, g, b, a) static_cast<uint32_t>( \
    static_cast<uint8_t>(r * 0xFF) << 0x00 | \
    static_cast<uint8_t>(g * 0xFF) << 0x08 | \
    static_cast<uint8_t>(b * 0xFF) << 0x10 | \
    static_cast<uint8_t>(a * 0xFF) << 0x18   \
)

#define BK_COLOR_RGB_FROM_FLOAT(r, g, b) static_cast<uint32_t>( \
    static_cast<uint8_t>(r * 0xFF) << 0x00 | \
    static_cast<uint8_t>(g * 0xFF) << 0x08 | \
    static_cast<uint8_t>(b * 0xFF) << 0x10 | \
    static_cast<uint8_t>(    0xFF) << 0x18   \
)

struct color_code {
    color_code(uint32_t value) : value(value) {}

    color_code(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0xFF)
        : value((r << 0x00) | (g << 0x08) | (b << 0x10) | (a << 0x18))
    {
    }

    template <typename T>
    inline static uint8_t truncate(T value) {
        return static_cast<uint8_t>((value < 0) ? (0) : ((value > 0xFF) ? (0xFF) : (value)));
    }

    template <typename T>
    color_code(T r, T g, T b, T a,
        typename std::enable_if<
            std::is_integral<T>::value &&
            !std::is_same<uint8_t, T>::value
        >::type* = nullptr
    )
        : color_code(truncate(r), truncate(g), truncate(b), truncate(a))
    {
    }

    template <typename T>
    color_code(T r, T g, T b, T a,
        typename std::enable_if<
            std::is_floating_point<T>::value &&
            !std::is_same<uint8_t, T>::value
        >::type* = nullptr
    )
        : color_code(truncate(r * 0xFF), truncate(g * 0xFF), truncate(b * 0xFF), truncate(a * 0xFF))
    {
    }

    uint8_t r() const { return static_cast<uint8_t>((value & 0x000000FF) >> 0x00); }
    uint8_t g() const { return static_cast<uint8_t>((value & 0x0000FF00) >> 0x08); }
    uint8_t b() const { return static_cast<uint8_t>((value & 0x00FF0000) >> 0x10); }
    uint8_t a() const { return static_cast<uint8_t>((value & 0xFF000000) >> 0x18); }

    uint32_t value;
};

struct color_f {
    color_f(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a)
    {
    }

    color_f(color_code code)
        : r(code.r() / 255.0f)
        , g(code.g() / 255.0f)
        , b(code.b() / 255.0f)
        , a(code.a() / 255.0f)
    {
    }

    color_f(uint32_t code)
        : color_f(color_code(code))
    {
    }

    operator color_code() const {
        return BK_COLOR_RGBA_FROM_FLOAT(r, g, b, a);
    }

    float r, g, b, a;
};

} // gfx
} // bklib