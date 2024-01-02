#pragma once

#include "GlRaii.h"

#include <glm/mat4x4.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace engine::graphics {

class ShaderBuilder {
  public:
    ShaderBuilder & attach( std::filesystem::path path, GLenum type );
    void build( gl::ShaderProgram & program );

  private:
    std::vector< std::unique_ptr< gl::Shader > > mShaders;
};

int findUniform( gl::ShaderProgram & program, const char * name );

void setUniform( int uniform, const glm::mat3 & mat );
void setUniform( int uniform, const glm::mat4 & mat );
void setUniform( int uniform, const glm::vec2 & v );
void setUniform( int uniform, const glm::vec3 & v );
void setUniform( int uniform, const glm::vec4 & v );
void setUniform( int uniform, int x );
void setUniform( int uniform, float x );

} // namespace engine::graphics
