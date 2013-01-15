#include "pch.hpp"
#include "gl.hpp"

#include <fstream>
#include <memory>

namespace gl = ::bklib::gl;

//==============================================================================
//! Shader.
//==============================================================================
gl::shader::shader(shader_type type)
    : id_(gl::create(type))
{
}
        
gl::shader::shader(shader_type type, GLchar const* source, GLint length)
    : shader(type)
{
    set_source(source, length);
}
        
gl::shader::shader(shader_type type, std::wstring file)
    : shader(type)
{
    std::ifstream in(file, std::ios::binary | std::ios::in | std::ios::ate);
    in.exceptions(std::ios::badbit | std::ios::failbit);

    auto const size = static_cast<GLint>(in.tellg());
    in.seekg(std::ios::beg, 0);

    std::unique_ptr<GLchar[]> buffer(new char[size]);
    in.read(buffer.get(), size);

    set_source(buffer.get(), size);
}

void gl::shader::set_source(GLchar const* source, GLint length) {
    ::glShaderSource(id_.value, 1, &source, &length);
}
        
void gl::shader::compile() {
    ::glCompileShader(id_.value);

    auto const len = get_<property::info_log_length>();
    std::unique_ptr<GLchar[]> buffer(new GLchar[len]);
    GLsizei result = 0;

    if (len != 0) {
        ::glGetShaderInfoLog(id_.value, len, &result, buffer.get());
        OutputDebugStringA(buffer.get());
    }
}

bool gl::shader::is_compiled() const {
    return get_<property::compile_status>() == GL_TRUE;
}

//==============================================================================
//! Program.
//==============================================================================
gl::program::program()
    : id_(gl::create())
{
}

void gl::program::attach(shader const& s) {
    ::glAttachShader(id_.value, s.id_.value);
}
        
void gl::program::use() {
    ::glUseProgram(id_.value);
}

void gl::program::link() {
    ::glLinkProgram(id_.value);

    auto const len = get_<property::info_log_length>();
    std::unique_ptr<GLchar[]> buffer(new GLchar[len]);
    GLsizei result = 0;

    if (len != 0) {
        ::glGetProgramInfoLog(id_.value, len, &result, buffer.get()); 
        ::OutputDebugStringA(buffer.get());
    }
}

gl::id::uniform gl::program::get_uniform_location(GLchar const* name) const {
    return id::uniform(
        ::glGetUniformLocation(id_.value, name)
    );
}
