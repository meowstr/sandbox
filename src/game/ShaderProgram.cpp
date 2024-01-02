#include "ShaderProgram.h"

#include "Logging.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>

namespace engine::graphics {

ShaderBuilder & ShaderBuilder::attach( std::filesystem::path path,
                                       GLenum type ) {
    path.make_preferred();
    INFO_LOG() << "Loading shader " << path.generic_string() << "."
               << std::endl;
    //  load file into string
    std::ifstream file( path );

    if ( !file ) {
        throw std::runtime_error( "Could not read shader file: " +
                                  path.string() );
    }

    gl::Shader * shader = new gl::Shader();
    shader->reset( glCreateShader( type ) );

    std::string source( ( std::istreambuf_iterator< char >( file ) ),
                        ( std::istreambuf_iterator< char >() ) );

    // load string into shader and compile
    const char * cSource = source.c_str();
    glShaderSource( *shader, 1, &cSource, nullptr );
    glCompileShader( *shader );

    // check if the shader compiled
    int success;
    static char infoLog[ 512 ];
    glGetShaderiv( *shader, GL_COMPILE_STATUS, &success );
    if ( !success ) {
        glGetShaderInfoLog( *shader, 512, nullptr, infoLog );
        throw std::runtime_error( "Shader failed to compile: " +
                                  std::string( infoLog ) );
    }

    mShaders.emplace_back( shader );

    return *this;
}

void ShaderBuilder::build( gl::ShaderProgram & program ) {
    // attach shaders
    for ( auto & shader : mShaders ) {
        glAttachShader( program, *shader );
    }

    glLinkProgram( program );

    // check if the shader compiled
    int success;
    static char infoLog[ 512 ];
    glGetProgramiv( program, GL_LINK_STATUS, &success );
    if ( !success ) {
        glGetProgramInfoLog( program, 512, nullptr, infoLog );
        throw std::runtime_error( "Shader program failed to link: " +
                                  std::string( infoLog ) );
    }
}

int findUniform( gl::ShaderProgram & program, const char * name ) {
    int i = glGetUniformLocation( program, name );
    if ( i < 0 ) {
        throw std::runtime_error( "Could not find uniform named \"" +
                                  std::string( name ) + "\"." );
    } else {
        return (unsigned int) i;
    }
}

void setUniform( int uniform, const glm::mat3 & mat ) {
    glUniformMatrix3fv( uniform, 1, GL_FALSE, glm::value_ptr( mat ) );
}

void setUniform( int uniform, const glm::mat4 & mat ) {
    glUniformMatrix4fv( uniform, 1, GL_FALSE, glm::value_ptr( mat ) );
}

void setUniform( int uniform, const glm::vec2 & v ) {
    glUniform2fv( uniform, 1, glm::value_ptr( v ) );
}

void setUniform( int uniform, const glm::vec3 & v ) {
    glUniform3fv( uniform, 1, glm::value_ptr( v ) );
}

void setUniform( int uniform, const glm::vec4 & v ) {
    glUniform4fv( uniform, 1, glm::value_ptr( v ) );
}

void setUniform( int uniform, int x ) {
    glUniform1i( uniform, x );
}

void setUniform( int uniform, float x ) {
    glUniform1f( uniform, x );
}

} // namespace engine::graphics
