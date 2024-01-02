#pragma once

#include "Math.h"

#include <vector>

namespace engine {

struct NavMesh {
    std::vector< math::Segment > internal;
    std::vector< math::Segment > boundary;

    std::vector< glm::vec2 > vertices;
};

void moveOnNavMesh( glm::vec2 & pos, glm::vec2 delta, const NavMesh & navMesh );

} // namespace engine
