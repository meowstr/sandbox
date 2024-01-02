#pragma once

#include "Vertex.h"

#include <vector>

namespace engine {

struct Mesh {
    std::vector< Vertex > vertices;

    // lower-bound vertex
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    // bounding box size
    glm::vec3 size = { 0.0f, 0.0f, 0.0f };
};
} // namespace engine
