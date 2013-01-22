////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Opengl related classes and functions.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <type_traits>
#include <array>
#include <vector>
#include <tuple>

#include <GL/glew.h>
#include <gl/GL.h>

#include "util/util.hpp" // for get_enum

namespace bklib {
namespace gl {

//==============================================================================
//! Check the current gl error state and break on error; otherwise, do nothing.
//==============================================================================
#define BK_GL_CHECK_ERROR do {              \
    auto const error_code = ::glGetError(); \
    if (error_code) __debugbreak();         \
} while(false)

//==============================================================================
//! Wrapper for opengl ids for type safety.
//==============================================================================
template <
    typename Tag,
    typename Storage = GLuint
>
struct identifier {
    typedef Tag     tag;
    typedef Storage type;

    explicit identifier(type value = 0) : value(value) {}

    type value;
};

//==============================================================================
//! opengl id types.
//==============================================================================
namespace id {
    typedef identifier<struct tag_program>      program;
    typedef identifier<struct tag_shader>       shader;
    typedef identifier<struct tag_vertex_array> vertex_array;
    typedef identifier<struct tag_buffer>       buffer;
    typedef identifier<struct tag_attribute>    attribute;
    typedef identifier<struct tag_uniform>      uniform;
    typedef identifier<struct tag_texture>      texture;
} // namespace id

////////////////////////////////////////////////////////////////////////////////
// Operations common to id types.
////////////////////////////////////////////////////////////////////////////////

//! Corresponds to glDelete* functions for 1 id.
template <typename T> void destroy(T id);
//! Corresponds to glDelete* functions for multiple ids.
template <typename T> void destroy(GLsizei, T* id);
//! Corresponds to glIs* functions .
template <typename T> bool is(T id);
//! Corresponds to glGenerate* functions for 1 id.
template <typename T> T generate();
//! Corresponds to glGenerate* functions for multiple ids.
template <typename T> std::vector<T> generate(GLsizei n);

///////////////////////////////////////////////////////////////////////////////
// Specializations for id::program.
///////////////////////////////////////////////////////////////////////////////
inline id::program create() {
    id::program const result(::glCreateProgram());
    BK_GL_CHECK_ERROR;

    return result;
}

template <>
inline void destroy<id::program>(id::program id) {
    ::glDeleteProgram(id.value);
    BK_GL_CHECK_ERROR;
}

template <>
inline bool is<id::program>(id::program id) {
    return ::glIsProgram(id.value) == GL_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Specializations id::shader.
////////////////////////////////////////////////////////////////////////////////

//==============================================================================
//! Shader types.
//==============================================================================
enum class shader_type : GLenum {
    vertex   = GL_VERTEX_SHADER,
	geometry = GL_GEOMETRY_SHADER,
	fragment = GL_FRAGMENT_SHADER,
};

inline id::shader create(shader_type type) {
    id::shader const result(::glCreateShader(get_enum_value(type)));
    BK_GL_CHECK_ERROR;

    return result;
}

template <>
inline void destroy<id::shader>(id::shader id) {
    ::glDeleteShader(id.value);
    BK_GL_CHECK_ERROR;
}

template <>
inline bool is<id::shader>(id::shader id) {
    return ::glIsShader(id.value) == GL_TRUE;
}
////////////////////////////////////////////////////////////////////////////////
// Specializations id::vertex_array.
////////////////////////////////////////////////////////////////////////////////
template <>
inline void destroy<id::vertex_array>(id::vertex_array id) {
    ::glDeleteVertexArrays(1, &id.value);
    BK_GL_CHECK_ERROR;
}

template <>
inline bool is<id::vertex_array>(id::vertex_array id) {
    return ::glIsVertexArray(id.value) == GL_TRUE;
}

template <>
inline id::vertex_array generate<id::vertex_array>() {
    GLuint id = 0;
    
    ::glGenVertexArrays(1, &id);
    BK_GL_CHECK_ERROR;

    return id::vertex_array(id);
}
    
template <>
inline std::vector<id::vertex_array> generate<id::vertex_array>(GLsizei n) {
    std::vector<id::vertex_array> ids(n, id::vertex_array(0));
    
    ::glGenVertexArrays(1, reinterpret_cast<GLuint*>(ids.data()));
    BK_GL_CHECK_ERROR;

    return ids;
}

////////////////////////////////////////////////////////////////////////////////
// Specializations id::buffer.
////////////////////////////////////////////////////////////////////////////////
template <>
inline void destroy<id::buffer>(id::buffer id) {
    ::glDeleteBuffers(1, &id.value);
    BK_GL_CHECK_ERROR;
}

template <>
inline bool is<id::buffer>(id::buffer id) {
    return ::glIsBuffer(id.value) == GL_TRUE;
}

template <>
inline id::buffer generate<id::buffer>() {
    GLuint id = 0;
    
    ::glGenBuffers(1, &id);
    BK_GL_CHECK_ERROR;

    return id::buffer(id);
}
    
template <>
inline std::vector<id::buffer> generate<id::buffer>(GLsizei n) {
    std::vector<id::buffer> ids(n, id::buffer(0));
    
    ::glGenBuffers(1, reinterpret_cast<GLuint*>(ids.data()));
    BK_GL_CHECK_ERROR;

    return ids;
}

////////////////////////////////////////////////////////////////////////////////
// Specializations id::texture.
////////////////////////////////////////////////////////////////////////////////
template <>
inline void destroy<id::texture>(id::texture id) {
    ::glDeleteTextures(1, &id.value);
    BK_GL_CHECK_ERROR;
}

template <>
inline bool is<id::texture>(id::texture id) {
    return ::glIsTexture(id.value) == GL_TRUE;
}

template <>
inline id::texture generate<id::texture>() {
    GLuint id = 0;
    
    ::glGenTextures(1, &id);
    BK_GL_CHECK_ERROR;

    return id::texture(id);
}
    
template <>
inline std::vector<id::texture> generate<id::texture>(GLsizei n) {
    std::vector<id::texture> ids(n, id::texture(0));
    
    ::glGenTextures(1, reinterpret_cast<GLuint*>(ids.data()));
    BK_GL_CHECK_ERROR;

    return ids;
}

//==============================================================================
//! Vertex attribute traits.
//! @t-param Type
//!     A data_traits<Size, Type> type.
//! @t-param Stride
//!     Offset between verticies in the buffer.
//! @t-param Offset
//!     Offset to next field in the buffer.
//! @t-param Normalized
//!     Normalize data?
//! @t-param Integral
//!     Integral data?
//==============================================================================
template <
    typename Type,
    GLsizei  Stride     = 0,
    size_t   Offset     = 0,
    bool     Normalized = false,
    bool     Integral   = false
>
struct attribute_traits {
    static auto const normalized = Normalized;
    static auto const gl_size    = Type::gl_size;
    static auto const gl_type    = Type::gl_type;
    static auto const stride     = Stride;
    static auto const offset     = Offset;
    static auto const integral   = Integral;
        
    typedef typename Type::type type;
    static auto const size = Type::size;

    //--------------------------------------------------------------------------
    //! Set the attributes for the active vertex array.
    //--------------------------------------------------------------------------
    static void set_attribute_pointer(id::attribute index) {
        if (integral) ::glVertexAttribIPointer(
            index.value,
            static_cast<GLint>(gl_size),
            static_cast<GLenum>(gl_type),
            stride,
            (GLvoid*)offset
        );
        else ::glVertexAttribPointer(
            index.value,
            static_cast<GLint>(gl_size),
            static_cast<GLenum>(gl_type),
            normalized ? GL_TRUE : GL_FALSE,
            stride,
            (GLvoid*)offset
        );
    }
};

//==============================================================================
//! opengl buffer definitions.
//==============================================================================
namespace buffer {
    //==========================================================================
    //! Element count.
    //==========================================================================    
    enum class size : GLint {
        size_1 = 1,
        size_2 = 2,
        size_3 = 3,
        size_4 = 4,
    };
    //==========================================================================
    //! Element data type.
    //==========================================================================
    enum class type : GLenum {
        byte_s           = GL_BYTE,
        byte_u           = GL_UNSIGNED_BYTE,
        short_s          = GL_SHORT,
        short_u          = GL_UNSIGNED_SHORT,
        int_s            = GL_INT,
        int_u            = GL_UNSIGNED_INT,
        fp_half          = GL_HALF_FLOAT,
        fp_single        = GL_FLOAT,
        fp_double        = GL_DOUBLE,
        int_2_10_10_10_s = GL_INT_2_10_10_10_REV,
        int_2_10_10_10_u = GL_UNSIGNED_INT_2_10_10_10_REV,
    };
    //==========================================================================
    //! Buffer binding target.
    //==========================================================================
    enum class target : GLenum {
        array              = GL_ARRAY_BUFFER,
        copy_read          = GL_COPY_READ_BUFFER,
        copy_write         = GL_COPY_WRITE_BUFFER,
        element_array      = GL_ELEMENT_ARRAY_BUFFER,
        pixel_pack         = GL_PIXEL_PACK_BUFFER,
        pixel_unpack       = GL_PIXEL_UNPACK_BUFFER,
        texture            = GL_TEXTURE_BUFFER,
        transform_feedback = GL_TRANSFORM_FEEDBACK_BUFFER,
        uniform            = GL_UNIFORM_BUFFER,
    };
    //==========================================================================
    //! Buffer usage type.
    //==========================================================================
    enum class usage : GLenum {
        stream_draw  = GL_STREAM_DRAW,
        stream_read  = GL_STREAM_READ,
        stream_copy  = GL_STREAM_COPY, 
        static_draw  = GL_STATIC_DRAW,
        static_read  = GL_STATIC_READ,
        static_copy  = GL_STATIC_COPY, 
        dynamic_draw = GL_DYNAMIC_DRAW,
        dynamic_read = GL_DYNAMIC_READ,
        dynamic_copy = GL_DYNAMIC_COPY,
    };            
    //==========================================================================
    //! Traits describing a buffer::type.
    //==========================================================================        
    template <buffer::type T> struct type_traits;

    //! Convenience macro to specialize type_traits.
    #define BK_GL_DECLARE_STORAGE_TYPE(ENUM, TYPE) \
    template <>                                    \
    struct type_traits<buffer::type::ENUM> {       \
        typedef TYPE type;                         \
        static auto const size = sizeof(type);     \
    }                                              

    BK_GL_DECLARE_STORAGE_TYPE(byte_s,    GLbyte);
    BK_GL_DECLARE_STORAGE_TYPE(byte_u,    GLubyte);
    BK_GL_DECLARE_STORAGE_TYPE(short_s,   GLshort);
    BK_GL_DECLARE_STORAGE_TYPE(short_u,   GLushort);
    BK_GL_DECLARE_STORAGE_TYPE(int_s,     GLint);
    BK_GL_DECLARE_STORAGE_TYPE(int_u,     GLuint);
    BK_GL_DECLARE_STORAGE_TYPE(fp_single, GLfloat);

    #undef BK_GL_DECLARE_STORAGE_TYPE
    //==========================================================================
    //! Data storage traits.
    //! @t-param Size
    //!     GL element count.
    //! @t-param Type
    //!     GL element data type.
    //==========================================================================    
    template <
        buffer::size Size = buffer::size::size_4,
        buffer::type Type = buffer::type::fp_single
    >
    struct data_traits {
        typedef type_traits<Type> traits;

        static auto const gl_size  = Size;
        static auto const gl_type  = Type;
        static auto const elements = enum_value<buffer::size, Size>::value;
        static auto const size     = traits::size * elements;
        
        typedef typename traits::type              element_type;
        typedef std::array<element_type, elements> type;
    };

    namespace detail {
        //======================================================================
        //! Calculate the stride.
        //======================================================================
        template <typename T, size_t N>
        struct data_traits_get_stride {
            static auto const value =
                data_traits_get_stride<T, N-1>::value +
                std::tuple_element<N, T>::type::size;
        };
        //! Base case
        template <typename T>
        struct data_traits_get_stride<T, 0> {
            static auto const value = std::tuple_element<0, T>::type::size;
        };
    

        //======================================================================
        //! Implementation for data_traits_set.
        //! @t-param T
        //!     A std::tuple of data_traits.
        //======================================================================
        template <typename T>
        struct data_traits_set_base {
            static auto const size = std::tuple_size<T>::value;
            typedef T traits;

            static auto const stride =
                detail::data_traits_get_stride<traits, size-1>::value;

            //------------------------------------------------------------------
            //! Offset of the Nth element.
            //------------------------------------------------------------------
            template <size_t N>
            struct offset_of {
                static auto const value =
                    offset_of<N-1>::value +
                    std::tuple_element<N-1, traits>::type::size;
            };
            //! Base case.
            template <> struct offset_of<0> {
                static size_t const value = 0;
            };

            //------------------------------------------------------------------
            //! Vertex attribute traits for the Nth element.
            //------------------------------------------------------------------
            template <
                size_t N,
                bool   Normalized = false,
                bool   Integral   = false
            >
            struct attribute_traits {
                typedef gl::attribute_traits<
                    typename std::tuple_element<N, traits>::type,
                    stride,
                    offset_of<N>::value,
                    Normalized,
                    Integral
                > type;
            };
        };
    } // namespace detail

    //==========================================================================
    //! A set of data_traits describing a vertex format.
    //! @t-param T
    //!     The head of the data_traits.
    //! @t-param Types
    //!     The tail of the data_traits.
    //==========================================================================
    template <typename T, typename... Types>
    struct data_traits_set
        : public detail::data_traits_set_base<std::tuple<T, Types...>>
    {
    };
    //! Base case.
    template <typename T>
    struct data_traits_set<T>
        : public detail::data_traits_set_base<std::tuple<T>>
    {
    };
} // namespace buffer

//==============================================================================
//! Render mode.
//==============================================================================
enum class mode : GLenum {
    points                   = GL_POINTS,
    line_strip               = GL_LINE_STRIP,
    line_loop                = GL_LINE_LOOP,
    lines                    = GL_LINES,
    line_strip_adjacency     = GL_LINE_STRIP_ADJACENCY,
    lines_adjacency          = GL_LINES_ADJACENCY,
    triangle_strip           = GL_TRIANGLE_STRIP,
    triangle_fan             = GL_TRIANGLE_FAN,
    triangles                = GL_TRIANGLES,
    triangle_strip_adjacency = GL_TRIANGLE_STRIP_ADJACENCY,
    triangles_adjacency      = GL_TRIANGLES_ADJACENCY,
    patches                  = GL_PATCHES,
};

class shader;
class program;
class vertex_array;

//==============================================================================
//! Shader.
//==============================================================================
class shader {
    friend program;
public:
    shader(shader_type type);
    shader(shader_type type, GLchar const* source, GLint length);
    shader(shader_type type, std::wstring file);

    void set_source(GLchar const* source, GLint length);
        
    void compile();

    bool is_compiled() const;
private:
    //--------------------------------------------------------------------------
    //! Shader properties.
    //--------------------------------------------------------------------------
    enum class property : GLenum {
        shader_type          = GL_SHADER_TYPE,
		delete_status        = GL_DELETE_STATUS,
		compile_status       = GL_COMPILE_STATUS,
		info_log_length      = GL_INFO_LOG_LENGTH,
		shader_source_length = GL_SHADER_SOURCE_LENGTH,
    };

    template <property P> GLint get_() const {
        GLint result = 0;
        ::glGetShaderiv(id_.value, enum_value<property, P>::value, &result);
        return result;
    }

    id::shader id_;
};
    
//==============================================================================
//! Program.
//==============================================================================
class program {
public:
    program();

    void attach(shader const& s);
        
    void use();

    void link();

    id::uniform get_uniform_location(GLchar const* name) const;

    void set_uniform(id::uniform index, glm::mat4 const& mat) {
        ::glUniformMatrix4fv(index.value, 1, GL_FALSE, &mat[0][0]);
    }
private:
    //--------------------------------------------------------------------------
    //! Program properties.
    //--------------------------------------------------------------------------
    enum class property : GLenum {
        delete_status            = GL_DELETE_STATUS,
        link_status              = GL_LINK_STATUS,
        validate_status          = GL_VALIDATE_STATUS,
        info_log_length          = GL_INFO_LOG_LENGTH,
        attached_shaders         = GL_ATTACHED_SHADERS,
        active_attributes        = GL_ACTIVE_ATTRIBUTES,
        active_attribute_max_len = GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
        active_uniforms          = GL_ACTIVE_UNIFORMS,
        active_uniforms_max_len  = GL_ACTIVE_UNIFORM_MAX_LENGTH,
    };

    template <property P> GLint get_() const {
        GLint result = 0;
        ::glGetProgramiv(id_.value, enum_value<property, P>::value, &result);
        return result;
    }

    id::program id_;
};

//==============================================================================
//! Vertex Array.
//==============================================================================
class vertex_array {
public:
    vertex_array()
        : id_(generate<id::vertex_array>())
    {
    }

    void bind() const {
        ::glBindVertexArray(id_.value);
    }

    void unbind() const {
        ::glBindVertexArray(0);
    }

    void bind_buffer(id::buffer id) {
        ::glBindBuffer(GL_ARRAY_BUFFER, id.value);
    }

    void unbind_buffer() {
        ::glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void enable_attribute(id::attribute index) {
        ::glEnableVertexAttribArray(index.value);
    }

    void disable_attribute(id::attribute index) {
        ::glDisableVertexAttribArray(index.value);
    }

    template <typename T>
    void set_attribute_pointer(id::attribute index) {
        T::set_attribute_pointer(index);
    }
private:
    id::vertex_array id_;
};

//==============================================================================
//! Texure object.
//==============================================================================
namespace texture {
    enum class target : GLenum {
        tex_1d             = GL_TEXTURE_1D,
        tex_2d             = GL_TEXTURE_2D,
        tex_3d             = GL_TEXTURE_3D,
        tex_1d_array       = GL_TEXTURE_1D_ARRAY,
        tex_2d_array       = GL_TEXTURE_2D_ARRAY,
        tex_rect           = GL_TEXTURE_RECTANGLE,
        tex_cube           = GL_TEXTURE_CUBE_MAP,
        tex_cube_array     = GL_TEXTURE_CUBE_MAP_ARRAY,
        tex_buffer         = GL_TEXTURE_BUFFER,
        tex_2d_multi       = GL_TEXTURE_2D_MULTISAMPLE,
        tex_2d_multi_array = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
    };

    enum class binding : GLenum {
        binding_1d = GL_TEXTURE_BINDING_1D,
        binding_2d = GL_TEXTURE_BINDING_2D,
        binding_3d = GL_TEXTURE_BINDING_3D, 
        //GL_TEXTURE_BINDING_CUBE_MAP,
        //GL_TEXTURE_BINDING_1D_ARRAY,
        //GL_TEXTURE_BINDING_2D_ARRAY,
        //GL_TEXTURE_BINDING_RECTANGLE,
        //GL_TEXTURE_BINDING_BUFFER,
        //GL_TEXTURE_BINDING_CUBE_MAP_ARRAY,
        //GL_TEXTURE_BINDING_2D_MULTISAMPLE,
        //GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY,
    };

    namespace detail {
        template <target T> binding get_binding();
        template <> inline binding get_binding<target::tex_1d>() { return binding::binding_1d; }
        template <> inline binding get_binding<target::tex_2d>() { return binding::binding_2d; }
    };

    enum class min_filter : GLint {
        nearest                = GL_NEAREST,
        linear                 = GL_LINEAR,
        nearest_mipmap_hearest = GL_NEAREST_MIPMAP_NEAREST,
        linear_mipmap_hearest  = GL_LINEAR_MIPMAP_NEAREST,
        nearest_mipmap_linear  = GL_NEAREST_MIPMAP_LINEAR,
        linear_mipmap_linear   = GL_LINEAR_MIPMAP_LINEAR,
    };

    enum class internal_format : GLint {
        r8   = GL_R8,
        r8ui = GL_R8UI,
    };

    enum class data_format : GLenum {
        ir   = GL_RED_INTEGER,
        r    = GL_RED,
        rg   = GL_RG,
        rgb  = GL_RGB,
        bgr  = GL_BGR,
        rgba = GL_RGBA,
        bgra = GL_BGRA,                
    };

    enum class data_type : GLenum {
        byte_u    = GL_UNSIGNED_BYTE,
        byte_s    = GL_BYTE,
        short_u   = GL_UNSIGNED_SHORT,
        short_s   = GL_SHORT,
        int_u     = GL_UNSIGNED_INT,
        int_s     = GL_INT,
        fp_single = GL_FLOAT,
        byte_332  = GL_UNSIGNED_BYTE_3_3_2,
        byte_233r = GL_UNSIGNED_BYTE_2_3_3_REV,
        //ubyte = GL_UNSIGNED_SHORT_5_6_5,
        //ubyte = GL_UNSIGNED_SHORT_5_6_5_REV,
        //ubyte = GL_UNSIGNED_SHORT_4_4_4_4,
        //ubyte = GL_UNSIGNED_SHORT_4_4_4_4_REV,
        //ubyte = GL_UNSIGNED_SHORT_5_5_5_1,
        //ubyte = GL_UNSIGNED_SHORT_1_5_5_5_REV,
        //ubyte = GL_UNSIGNED_INT_8_8_8_8,
        //ubyte = GL_UNSIGNED_INT_8_8_8_8_REV,
        //ubyte = GL_UNSIGNED_INT_10_10_10_2,
        //ubyte = GL_UNSIGNED_INT_2_10_10_10_REV,
    };

} //namespace texture

template <texture::target Target>
struct texture_object {
    static auto const target = Target;

    texture_object()
        : id_(generate<id::texture>())
    {
        BK_GL_CHECK_ERROR;
    }

    void bind() {
        ::glBindTexture(get_enum_value(target), id_.value);
        BK_GL_CHECK_ERROR;
    }
    
    void create(
        unsigned                 w,
        unsigned                 h,
        texture::internal_format internal,
        texture::data_format     format,
        texture::data_type       type,
        void const*              data         = nullptr,
        unsigned                 mipmap_level = 0
    ) {
        BK_ASSERT(is_bound());

        ::glTexImage2D(
            get_enum_value(target),
            mipmap_level,
            get_enum_value(internal),
            w, h,
            0,
            get_enum_value(format),
            get_enum_value(type),
            data
        );

        BK_GL_CHECK_ERROR;
    }

    void update(
        signed               xoff,
        signed               yoff,
        unsigned             w,
        unsigned             h,
        texture::data_format format,
        texture::data_type   type,
        void const*          data         = nullptr,
        unsigned             mipmap_level = 0
    ) {
        BK_ASSERT(is_bound());

        ::glTexSubImage2D(
            get_enum_value(target),
            mipmap_level,
            xoff,
            yoff,
            w,
            h,
            get_enum_value(format),
            get_enum_value(type),
            data
        );

        BK_GL_CHECK_ERROR;
    }

    void set_min_filter(texture::min_filter const filter) {
        BK_ASSERT(is_bound());

        ::glTexParameteri(
            get_enum_value(target),
            GL_TEXTURE_MIN_FILTER,
            get_enum_value(filter)
        );

        BK_GL_CHECK_ERROR;
    }

    bool is_bound() const {
        GLint result = 0;

        ::glGetIntegerv(
            get_enum_value(texture::detail::get_binding<target>()),
            &result
        );

        return static_cast<GLuint>(result) == id_.value;
    }
private:
    id::texture id_;
};

//==============================================================================
//! Buffer object.
//==============================================================================
struct buffer_object_base {
    enum class property : GLenum {
        array_binding              = GL_ARRAY_BUFFER_BINDING
      , atomic_counter_binding     = GL_ATOMIC_COUNTER_BUFFER_BINDING
      //, copy_read_binding          = GL_COPY_READ_BUFFER_BINDING
      //, copy_write_binding         = GL_COPY_WRITE_BUFFER_BINDING
      , draw_indirect_binding      = GL_DRAW_INDIRECT_BUFFER_BINDING
      , dispatch_indirect_binding  = GL_DISPATCH_INDIRECT_BUFFER_BINDING
      , element_array_binding      = GL_ELEMENT_ARRAY_BUFFER_BINDING
      , pixel_pack_binding         = GL_PIXEL_PACK_BUFFER_BINDING
      , pixel_unpack_binding       = GL_PIXEL_UNPACK_BUFFER_BINDING
      , shader_storage_binding     = GL_SHADER_STORAGE_BUFFER_BINDING
      , transform_feedback_binding = GL_TRANSFORM_FEEDBACK_BUFFER_BINDING
      , uniform_binding            = GL_UNIFORM_BUFFER_BINDING
    };

    static GLuint get_property(property p) {
        GLint result = 0;
        ::glGetIntegerv((GLenum)p, &result);

        return static_cast<GLuint>(result);
    }
};

template <
      typename       T
    , buffer::target Target = buffer::target::array
    , buffer::usage  Usage  = buffer::usage::dynamic_draw
>
class buffer_object;

//==============================================================================
//! Array buffer object specialization.
//==============================================================================
template <typename T>
class buffer_object<
    T
    , buffer::target::array
    , buffer::usage::dynamic_draw
> : public buffer_object_base {
public:
    static auto const target = buffer::target::array;
    static auto const usage  = buffer::usage::dynamic_draw;    
    typedef T element_type;

    explicit buffer_object(size_t elements = 0)
        : id_(gl::generate<id::buffer>())
        , elements_(0)
    {
        if (elements) {
            allocate(elements);
        }
    }

    void allocate(size_t elements, element_type const* data = nullptr) {
        BK_ASSERT(elements > 0);
        BK_ASSERT(elements_ == 0);

        bind();
        buffer_(elements * sizeof(T), data);
    }

    template <typename U>
    void update(size_t element, U const& data, size_t offset = 0) {
        static_assert(sizeof(U) <= sizeof(T), "type is too large.");
        BK_ASSERT(offset + sizeof(U) <= sizeof(T));

        bind();
        buffer_(element * sizeof(T) + offset, sizeof(U), &data);
    }

    void bind() {
        ::glBindBuffer(get_enum_value(target), id_.value);
        BK_GL_CHECK_ERROR;
    };

    void unbind() {
        BK_ASSERT(is_bound());

        ::glBindBuffer(get_enum_value(target), 0);
        BK_GL_CHECK_ERROR;
    };

    bool is_bound() const {
        return get_property(property::array_binding) == id_.value;
    }
private:
    void buffer_(size_t size, void const* data) {
        BK_ASSERT(is_bound());

        ::glBufferData(
              get_enum_value(target)
            , size
            , data
            , get_enum_value(usage)
        );
        BK_GL_CHECK_ERROR;
    };

    void buffer_(size_t offset, size_t size, void const* data) {
        BK_ASSERT(is_bound());

        ::glBufferSubData(
              get_enum_value(target)
            , offset
            , size
            , data
        );
        BK_GL_CHECK_ERROR;
    }

    id::buffer id_;
    size_t     elements_;
};

} // namespace gl
} // namespace bklib
