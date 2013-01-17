#include "pch.hpp"
#include "text.hpp"

#include "gfx/gl.hpp"

#include <Shlobj.h> //font path

namespace gfx = ::bklib::gfx;
namespace gl  = ::bklib::gl;

//template <typename Heuristic>
struct glyph_cache {
    static unsigned const TEX_W = 1024;
    static unsigned const TEX_H = 1024;

    static unsigned const CELL_W = 16;
    static unsigned const CELL_H = 16;

    static unsigned const CELL_COUNT_X = TEX_W / CELL_W;
    static unsigned const CELL_COUNT_Y = TEX_H / CELL_H;

    static unsigned const CELL_COUNT = CELL_COUNT_X * CELL_COUNT_Y;

    typedef uint32_t codepoint;

    static codepoint const FREE_SLOT = 0;

    struct glyph_info {
        codepoint code; // codepoint of the glyph
        uint16_t  w;    // width of the glyph
        uint16_t  h;    // height of the glyph
    };

    struct cache_info {
        codepoint code;
        size_t    index;

        bool operator<(cache_info const rhs) const {
            return code < rhs.code;
        }
    };

    static size_t pos_to_index(unsigned const x, unsigned const y) {
        BK_ASSERT(x < CELL_COUNT_X && y < CELL_COUNT_Y);
        return x + y * CELL_COUNT_X;
    }

    static unsigned index_to_x(size_t const index) {
        BK_ASSERT(index < CELL_COUNT);
        return index % CELL_COUNT_X;
    }
    
    static unsigned index_to_y(size_t const index) {
        BK_ASSERT(index < CELL_COUNT);
        return index / CELL_COUNT_X;
    }

    typedef std::array<glyph_info, CELL_COUNT> cache_container_t;
    typedef std::array<cache_info, CELL_COUNT> map_container_t;
public:
    typedef std::function<glyph_info (codepoint code, uint16_t x, uint16_t y)> fill_function_t;

    explicit glyph_cache(fill_function_t fill_function)
        : next_free_(0)
        , fill_cache_(fill_function)
    {
        cache_info const info = { 0, CELL_COUNT };
        std::fill(std::begin(map_), std::end(map_), info);
    }

    size_t get_free_slot() {
        return (next_free_ < CELL_COUNT) ? (next_free_++) : (CELL_COUNT);
    }

    map_container_t::iterator find_codepoint(codepoint const code) {
        return std::find_if(
            std::begin(map_),
            std::end(map_),
            [code](cache_info i) { return i.code == code; }
        );
    }
    
    struct record {
        glyph_info info;
        uint16_t   x;
        uint16_t   y;
    };

    record get(codepoint const code) {
        auto const make_record = [&](size_t index) {
            record const result = {
                cache_[index],
                index_to_x(index),
                index_to_y(index)
            };

            return result;
        };
        
        auto const where = find_codepoint(code);
        if (where != std::end(map_)) {
            return make_record(where->index);
        }

        size_t index = get_free_slot();
        if (index != CELL_COUNT) {
            // insert at the end of the cache -- slots at the beginning will
            // all be empty.
            auto const where = (CELL_COUNT - 1) - index;
            BK_ASSERT(map_[where].index == CELL_COUNT);

            map_[where].code  = code;
            map_[where].index = index;

            // only the portion that are not empty need be sorted.
            std::sort(std::begin(map_) + where, std::end(map_));

            auto const info = fill_cache_(code, index_to_x(index), index_to_y(index));
            BK_ASSERT(info.code == code);

            cache_[index].code = code;
            cache_[index].w    = info.w;
            cache_[index].h    = info.h;

            auto const result = make_record(index);
            return result;
        }

        
    }
private:

    //iterator get_pos(codepoint p) {
    //    return std::find_if(
    //        std::begin(map),
    //        std::end(map),
    //        [p](mapping_t const& x) {
    //            x.code == p;
    //        }
    //    );
    //}

    map_container_t   map_;   // codepoint -> cache index
    cache_container_t cache_; // cache data
    size_t            next_free_;
    fill_function_t   fill_cache_;
    //Heuristic   replacement_;
    // 
};


//==============================================================================
//! 
//==============================================================================
struct glyph_vertex {
    //--------------------------------------------------------------------------
    // (x, y) position.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_2,
        gl::buffer::type::short_s
    > position_data;
    //--------------------------------------------------------------------------
    // (r, g, b, a) color.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_4,
        gl::buffer::type::byte_u
    > color_data;
    //--------------------------------------------------------------------------
    // (s, t) texture coordinates.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_2,
        gl::buffer::type::short_u
    > tex_coord_data;

    enum class attribute {
        position,
        color,
        tex_coord,
    };

    typedef gl::buffer::data_traits_set<
        position_data
      , color_data
      , tex_coord_data
    > data_set;

    typedef data_set::attribute_traits<0>::type       position_t;
    typedef data_set::attribute_traits<1, true>::type color_t;
    typedef data_set::attribute_traits<2>::type       tex_coord_t;

    static size_t const size = data_set::stride;

    position_t::type  position;
    color_t::type     color;
    tex_coord_t::type tex_coord;
};

struct glyph_rect {
    static size_t const vertex_count = 4;
    typedef glyph_vertex vertex;

    glyph_rect(short x, short y, short tx, short ty) {


    }

    std::array<vertex, vertex_count> vertices;
};


gfx::font_renderer::font_renderer() {
    static char const FONT_NAME[] = "meiryo.ttc";

    PWSTR font_path = nullptr;
    if (FAILED(
        ::SHGetKnownFolderPath(FOLDERID_Fonts, 0, 0, &font_path)
    )) {
        BK_TODO_BREAK;
    }

    BK_ON_SCOPE_EXIT({
        ::CoTaskMemFree(font_path);
    });

    bklib::utf8string const font = 
        bklib::utf8_16_converter().to_bytes(font_path) + "/" + FONT_NAME;

    auto result = FT_Init_FreeType(&library_);
    if (result) {
        BK_TODO_BREAK;
    }
        
    result = FT_New_Face(
        library_,
        font.c_str(),
        0,
        &face_
    );
        
    if (result == FT_Err_Unknown_File_Format) {
        BK_TODO_BREAK;
    } else if (result) {
        BK_TODO_BREAK;
    }

    result = FT_Set_Pixel_Sizes(face_, 0, 16);
    if (result) {
        BK_TODO_BREAK;
    }
}

void gfx::font_renderer::draw_text(bklib::utf8string const& string) {
    FT_GlyphSlot slot = face_->glyph;

    int pen_x = 100;
    int pen_y = 100;

    for (auto c : string) {
        auto result = FT_Load_Char(face_, c, FT_LOAD_DEFAULT);



        pen_x += slot->advance.x >> 6;
        pen_y += slot->advance.y >> 6;
    }
}

void gfx::font_renderer::init_text() {
    GLuint texture = 0;
    ::glGenTextures(1, &texture);
    ::glBindTexture(GL_TEXTURE_2D, texture);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    ::glTexImage2D(
        GL_TEXTURE_2D, // target
        0,  // level, 0 = base, no minimap,
        GL_R8UI, // internalformat
        256,  // width
        256,  // height
        0,  // border, always 0 in OpenGL ES
        GL_RED,  // format
        GL_UNSIGNED_BYTE, // type
        nullptr
    );

    auto const fill_f = [&](glyph_cache::codepoint code, uint16_t x, uint16_t y) {
        FT_GlyphSlot const slot = face_->glyph;
        auto result = FT_Load_Char(face_, code, FT_LOAD_RENDER);
        if (result) {
            BK_TODO_BREAK;
        }

        ::glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x*16, y*16, // x, y
            16, 16,     // w, h
            GL_RED,
            GL_UNSIGNED_BYTE,
            slot->bitmap.buffer
        );

        glyph_cache::glyph_info const info = {
            code, slot->bitmap.width, slot->bitmap.rows
        };

        return info;
    };

    glyph_cache cache(fill_f);

    for (unsigned i = 0; i < glyph_cache::CELL_COUNT; ++i) {
        glyph_cache::codepoint const code = std::rand() % 0xFFFF + 20;
        auto const record = cache.get(code);
    }
}

