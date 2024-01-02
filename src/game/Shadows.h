#pragma once

#include "GlRaii.h"
#include "Vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace engine::graphics {

struct ShadowMesh {
    std::vector< glm::vec3 > vertices;
};

struct ShadowMap {
    gl::Texture depthTexture;
    gl::Framebuffer depthBuffer;
};

struct ShadowMapShader {
    gl::ShaderProgram program;
};

// TODO: unfinished
void generateShadowMeshFromPoint( ShadowMesh & mesh,
                                  const std::vector< Vertex > & vertices,
                                  glm::vec3 pointLightPosition );

void initShadowMap( ShadowMap & map, int width, int height );
void useShadowMap( ShadowMap & map, const ShadowMapShader & shader,
                   glm::mat4 combinedMatrix );

} // namespace engine::graphics
