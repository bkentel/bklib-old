#pragma once

#include "gfx/gfx.hpp"
#include "gfx/gl.hpp"
#include "util/pool_allocator.hpp"
#include "common/math.hpp"

namespace bklib {
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
    // (s, t, corner_t, 0) texture coordinates.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_4,
        gl::buffer::type::short_u
    > tex_coord_data;
    //--------------------------------------------------------------------------
    // (width, height, w, h) window space dimensions.
    //--------------------------------------------------------------------------
    typedef gl::buffer::data_traits<
        gl::buffer::size::size_4,
        gl::buffer::type::short_u
    > dimensions_data;

    enum class attribute {
        position,
        color,
        tex_coord,
        dimensions,
    };

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
    typedef color<
        vertex::color_data::element_type,
        vertex::color_data::elements
    >          color;
    typedef math::rect<vertex::tex_coord_data::element_type> tex_rect;

    static size_t const vertex_count = 4;
    static size_t const size         = vertex::size * vertex_count;

    static color    const default_color;
    static tex_rect const default_tex_coord;

    enum class corner : size_t {
        top_left      = 3,
        top_right     = 1,
        bottom_left   = 2,
        bottom_right  = 0,
    };

    enum class corner_type {
        sharp, round
    };

    vertex& get_corner_(corner c) {
        auto const i = static_cast<std::underlying_type<corner>::type>(c);
        return vertices[i];
    }

    template <corner C>
    void set_color(color c) {
        auto& v = get_corner_(C);

        v.color[0] = gfx::r(c);
        v.color[1] = gfx::g(c);
        v.color[2] = gfx::b(c);
        v.color[3] = gfx::a(c);
    }

    void set_color(color c) {
        set_color<corner::bottom_right>(c);
        set_color<corner::top_right>(c);
        set_color<corner::bottom_left>(c);
        set_color<corner::top_left>(c);
    }

    template <corner C>
    void set_corner_type(corner_type type) {
        auto& v = get_corner_(C);
        v.tex_coord[2] = (type == corner_type::sharp ? 0 : 1);
    }

    void set_corner_type(corner_type type) {
        set_corner_type<corner::bottom_right>(type);
        set_corner_type<corner::top_right>(type);
        set_corner_type<corner::bottom_left>(type);
        set_corner_type<corner::top_left>(type);
    }
        
    template <corner C> void set_position(rect r);

    template <> void set_position<corner::top_left>(rect r) {
        auto& v = get_corner_(corner::top_left);

        v.position[0] = r.left;
        v.position[1] = r.top;     
        
        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = 0;
        v.dimensions[3] = 0;        
    }

    template <> void set_position<corner::top_right>(rect r) {
        auto& v = get_corner_(corner::top_right);

        v.position[0] = r.right;
        v.position[1] = r.top;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = r.width();
        v.dimensions[3] = 0;       
    }

    template <> void set_position<corner::bottom_left>(rect r) {
        auto& v = get_corner_(corner::bottom_left);

        v.position[0] = r.left;
        v.position[1] = r.bottom;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = 0;
        v.dimensions[3] = r.height();
    }

    template <> void set_position<corner::bottom_right>(rect r) {
        auto& v = get_corner_(corner::bottom_right);

        v.position[0] = r.right;
        v.position[1] = r.bottom;

        v.dimensions[0] = r.width();
        v.dimensions[1] = r.height();
        v.dimensions[2] = r.width();
        v.dimensions[3] = r.height();
    }

    explicit rect_data(rect r) {
        set_position<corner::bottom_right>(r);
        set_position<corner::top_right>(r);
        set_position<corner::bottom_left>(r);
        set_position<corner::top_left>(r);
    }

    //explicit
    //rect_data( rect r,
    //           color       tl_color        = default_color,
    //           corner_type tl_corner_style = corner_type::round,
    //           color       tr_color        = default_color,
    //           corner_type tr_corner_style = corner_type::round,
    //           color       bl_color        = default_color,
    //           corner_type bl_corner_style = corner_type::round,
    //           color       br_color        = default_color,
    //           corner_type br_corner_style = corner_type::round
    //) {
    //    set_position<corner::bottom_right>(r);
    //    set_color<corner::bottom_right>(tl_color);
    //    set_corner_type<corner::bottom_right>(tl_corner_style);

    //    set_position<corner::top_right>(r);
    //    set_color<corner::top_right>(tr_color);
    //    set_corner_type<corner::top_right>(tr_corner_style);

    //    set_position<corner::bottom_left>(r);
    //    set_color<corner::bottom_left>(bl_color);
    //    set_corner_type<corner::bottom_left>(bl_corner_style);

    //    set_position<corner::top_left>(r);
    //    set_color<corner::top_left>(br_color);
    //    set_corner_type<corner::top_left>(br_corner_style);
    //}

    rect_data() {
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

    typedef rect_data::rect   rect;
    typedef rect_data::color  color;
    typedef rect_data::corner corner;

    typedef rect_data rect_info;

    renderer2d();

    void begin_draw() {
        program_.use();
    }

    void end_draw() {
    }

    handle create_rect(rect_info const& info) {
        return rects_.alloc(info);
    }

    handle create_rect(
        rect_data::rect     rect,
        rect_data::color    c0  = rect_data::default_color,
        rect_data::color    c1  = rect_data::default_color,
        rect_data::color    c2  = rect_data::default_color,
        rect_data::color    c3  = rect_data::default_color,
        rect_data::tex_rect tex = rect_data::default_tex_coord
    ) {
        return rects_.alloc(
            rect_data(rect)
        );
    }

    void update_rect(handle h, rect r);

    template <corner C>
    void update_rect(handle h, color c) {
        rect_data data;
        data.set_vertex<C>(c);
        
        static auto const offset = rect_data::vertex::color_t::offset;
        static auto const stride = rect_data::vertex::color_t::stride;

        static auto const i = static_cast<size_t>(C);

        rects_.update<i * stride + offset>(h, data.vertices[i].color);
    }

    void draw_rect(handle rect) const;

    void set_viewport(unsigned w, unsigned h);
private:
    glm::mat4 model_mat_;
    glm::mat4 view_mat_;
    glm::mat4 proj_mat_;
    glm::mat4 mvp_mat_;

    gl::id::uniform mvp_loc_;
    gl::id::uniform border_size_loc_;
    gl::id::uniform corner_radius_loc_;

    gl::program program_;
    gl::shader  vert_shader_;
    gl::shader  frag_shader_;

    gl::vertex_array rect_array_;
    gl::vertex_array glyph_array_;

    pool_allocator<rect_data, gl::buffer_object<rect_data>> rects_;
    pool_allocator<rect_data, gl::buffer_object<rect_data>> glyph_rects_;
}; //renderer2d

struct render_state_2d {
    static GLuint const ATTR_POS_LOC = 0;
    static GLuint const ATTR_COL_LOC = 1;
    static GLuint const ATTR_TEX_LOC = 2;
    static GLuint const ATTR_DIM_LOC = 3;

    gl::program program;
    gl::shader  vert_shader;
    gl::shader  frag_shader;

    gl::id::attribute attr_pos;
    gl::id::attribute attr_col;
    gl::id::attribute attr_tex;
    gl::id::attribute attr_dim;

    glm::mat4 model_mat;
    glm::mat4 view_mat;
    glm::mat4 proj_mat;
    glm::mat4 mvp_mat;

    gl::id::uniform mvp_loc;
    
    gl::id::uniform render_type_loc;

    gl::id::uniform corner_radius_loc;
    gl::id::uniform gradient_steps_loc;
    gl::id::uniform border_size_loc;

    gl::id::uniform base_texture_loc;
};

} // namespace gfx
} // namespace bklib
