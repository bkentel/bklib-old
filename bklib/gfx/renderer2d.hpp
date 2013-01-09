#pragma once

#include <gfx/gl.hpp>
#include <util/pool_allocator.hpp>
#include <common/math.hpp>

namespace bklib {

//==============================================================================
//! 
//==============================================================================
template <
    typename T,
    bklib::gl::buffer::target Target = bklib::gl::buffer::target::array,
    bklib::gl::buffer::usage  Usage  = bklib::gl::buffer::usage::dynamic_draw
>
struct gl_buffer {
public:
    typedef T&       reference;
    typedef T const& const_reference;

    gl_buffer(size_t elements)
        : size_(sizeof(T)*elements)
    {
        buffer_.bind();
        buffer_.buffer(size_, nullptr, Usage);
    }

    void update(size_t element, const_reference value) {
        buffer_.bind();
        buffer_.buffer(element * sizeof(T), sizeof(T), &value);
    }
    
    void bind() {
        buffer_.bind();
    }

    void resize();
    
    void free();
private:
    bklib::gl::buffer_object<Target> buffer_;
    size_t size_;
};


namespace gfx {

//==============================================================================
//! 
//==============================================================================
struct rect_vertex {
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
    // (u, v) texture coordinates.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_2,
        gl::buffer::type::short_u
    > tex_coord_data;
    //--------------------------------------------------------------------------
    // (width, height, w, h) window space dimensions.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_4,
        gl::buffer::type::short_u
    > dimensions_data;

    typedef gl::buffer::data_traits_set<
        position_data,
        color_data,
        tex_coord_data,
        dimensions_data
    > data_set;

    typedef data_set::attribute_traits<0>::type       position_t;
    typedef data_set::attribute_traits<1, true>::type color_t;
    typedef data_set::attribute_traits<2>::type       tex_coord_t;
    typedef data_set::attribute_traits<3>::type       dimensions_t;

    static size_t const size = data_set::stride;

    position_t::type   position;
    color_t::type      color;
    tex_coord_t::type  tex_coord;
    dimensions_t::type dimensions;
};

template <typename T = uint8_t>
struct color {
    T r, g, b, a;
};

struct color32 {
    union {
        color<uint8_t> c;
        uint32_t       value;
    };
};

//==============================================================================
//! 
//==============================================================================
    //    ____                                                                    
    // 3 |   /| 1                                                                  
    //   |  / |                                                                  
    //   | /  |                                                                   
    // 2 |/___| 0                                                                 
    //   
struct rect_data {
    typedef rect_vertex                                      vertex;
    typedef math::rect<vertex::position_data::element_type>  rect;
    typedef color<vertex::color_data::element_type>          color;
    typedef math::rect<vertex::tex_coord_data::element_type> tex_rect;

    static size_t const vertex_count = 4;
    static size_t const size         = vertex::size * vertex_count;

    static color    const default_color;
    static tex_rect const default_tex_coord;

    enum class corner {
        top_left      = 3,
        top_right     = 1,
        bottom_left   = 2,
        bottom_right  = 0,
    };

    template <corner C>
    void set_vertex(color c) {
        static auto const i = static_cast<size_t>(C);
        auto& v = vertices[i];

        v.color[0] = c.r;
        v.color[1] = c.g;
        v.color[2] = c.b;
        v.color[3] = c.a;
    }
        
    template <corner C>
    void set_vertex(rect r);

    template <> void set_vertex<corner::top_left>(rect r) {
        static auto const i = static_cast<size_t>(corner::top_left);
        auto& v = vertices[i];

        v.position[0] = r.left;
        v.position[1] = r.top;     
        
        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = 0;
        v.dimensions[3] = 0;        
    }

    template <> void set_vertex<corner::top_right>(rect r) {
        static auto const i = static_cast<size_t>(corner::top_right);
        auto& v = vertices[i];

        v.position[0] = r.right;
        v.position[1] = r.top;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = r.width();
        v.dimensions[3] = 0;       
    }

    template <> void set_vertex<corner::bottom_left>(rect r) {
        static auto const i = static_cast<size_t>(corner::bottom_left);
        auto& v = vertices[i];

        v.position[0] = r.left;
        v.position[1] = r.bottom;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = 0;
        v.dimensions[3] = r.height();
    }

    template <> void set_vertex<corner::bottom_right>(rect r) {
        static auto const i = static_cast<size_t>(corner::bottom_right);
        auto& v = vertices[i];

        v.position[0] = r.right;
        v.position[1] = r.bottom;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = r.width();
        v.dimensions[3] = r.height();
    }

    rect_data(rect r, color c0, color c1, color c2, color c3) {
        set_vertex<corner::bottom_right>(r);
        set_vertex<corner::bottom_right>(c0);

        set_vertex<corner::top_right>(r);
        set_vertex<corner::top_right>(c1);

        set_vertex<corner::bottom_left>(r);
        set_vertex<corner::bottom_left>(c2);

        set_vertex<corner::top_left>(r);
        set_vertex<corner::top_left>(c3);
    }

    std::array<vertex, vertex_count> vertices;
};



//==============================================================================
//! 
//==============================================================================

class renderer2d {
public:
    enum class primitive {
        rectangle,
    };

    typedef pool_allocator_base::allocation handle;

    typedef rect_data::rect  rect;
    typedef rect_data::color color;

    renderer2d();

    handle create_rect(
        rect_data::rect     rect,
        rect_data::color    c0  = rect_data::default_color,
        rect_data::color    c1  = rect_data::default_color,
        rect_data::color    c2  = rect_data::default_color,
        rect_data::color    c3  = rect_data::default_color,
        rect_data::tex_rect tex = rect_data::default_tex_coord
    ) {
        return rects_.alloc(
            rect_data(rect, c0, c1, c2, c3)
        );
    }

    void update_rect() {
    }

    void draw_rect(handle rect) {
        gl::id::attribute position(0);
        gl::id::attribute color(1);
        gl::id::attribute tex_coord(2);
        gl::id::attribute dimensions(3);

        auto const i = rects_.block_index(rect);

        rect_array_.bind();
        
        rects_.buffer().bind();

        rect_array_.enable_attribute(position);
        rect_array_.enable_attribute(color);
        rect_array_.enable_attribute(dimensions);

        rect_array_.set_attribute_pointer<rect_data::vertex::position_t>(position);
        rect_array_.set_attribute_pointer<rect_data::vertex::color_t>(color);
        rect_array_.set_attribute_pointer<rect_data::vertex::dimensions_t>(dimensions);

        ::glDrawArrays(
            GL_TRIANGLE_STRIP, i*4, 4
        );

        rect_array_.disable_attribute(position);
        rect_array_.disable_attribute(color);
        rect_array_.disable_attribute(dimensions);
    }

private:
    gl::vertex_array rect_array_;
    pool_allocator<rect_data, gl_buffer<rect_data>> rects_;
}; //renderer2d

} // namespace gfx
} // namespace bklib
