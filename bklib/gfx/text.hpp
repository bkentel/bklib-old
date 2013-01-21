#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gfx/gl.hpp"

namespace bklib {

template <
    size_t N,
    typename value_t,
    typename info_t
>
struct cache {
public:
    static size_t const size = N;

    typedef value_t value_t;
    typedef info_t  info_t;

    struct map_t {
        value_t value;
        size_t  index;
    };

    struct cache_t {
        value_t value;
        info_t  info;
    };

    typedef std::function<info_t (size_t where, value_t value)> on_cache_f;
public:   
    explicit cache(on_cache_f on_cache)
        : free_index_(size)
        , on_cache_(on_cache)
    {
        if (std::is_pod<value_t>::value) {
            std::memset(map_.data(), 0, sizeof(map_t)*N);
        } else {
            map_t const value = { value_t(), 0 };
            std::fill(std::begin(map_), std::end(map_), value);
        }
    }

    size_t get(value_t const value) {
        static auto const comp_f = [](map_t const& a, map_t const& b) {
            return a.value < b.value;
        };

        auto const where = [&] {
            map_t const v = { value, 0 };
            return std::lower_bound(
                std::begin(map_), std::end(map_), v, comp_f
            );
        }();

        if ((where != std::end(map_)) && (where->value == value)) {
            return std::distance(std::begin(map_), where);
        }

        bool const is_full = (free_index_ == 0);
        size_t const index = is_full ? 
            (std::rand() % size) :
            --free_index_;

        auto const result = is_full ?
            map_[index].index :
            size - (free_index_ + 1);

        map_[index].value    = value;
        map_[index].index    = result;
        cache_[result].value = value;
        cache_[result].info  = on_cache_(result, value);

        auto it = std::begin(map_) + index;

        if (it > where) {
            std::sort(where, ++it, comp_f);
        } else if (it < where) {
            std::sort(it, where, comp_f);
        }

        return result;
    }

    cache_t const& operator[](size_t const where) const {
        BK_ASSERT(where < size);
        return cache_[where];
    }
private:
    std::array<map_t,   size> map_;
    std::array<cache_t, size> cache_;

    size_t     free_index_;
    on_cache_f on_cache_;
};

namespace gfx {

class font_renderer {
public:
    ~font_renderer() {}
    font_renderer();

    void draw_text(utf8string const& string);
    void draw_text(platform_string const string) {}
private:
    static size_t const TEX_SIZE    = 1024;
    static size_t const CELL_SIZE   = 16;
    static size_t const CELL_SIZE_X = (TEX_SIZE / CELL_SIZE);
    static size_t const CELL_SIZE_Y = (TEX_SIZE / CELL_SIZE);
    static size_t const CELL_COUNT  = (CELL_SIZE_X * CELL_SIZE_Y);

    FT_Glyph_Metrics on_fill_cache_(size_t where, utf32codepoint code);

    cache<CELL_COUNT, utf32codepoint, FT_Glyph_Metrics> glyph_cache_;

    FT_Library library_;
    FT_Face    face_;

    gl::texture_object<gl::texture::target::tex_2d> cache_texture_;
};

} // namespace gfx
} // namespace bklib
