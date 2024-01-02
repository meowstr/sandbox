#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace engine {

#pragma pack( push, 1 )
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};
#pragma pack( pop )

} // namespace engine
