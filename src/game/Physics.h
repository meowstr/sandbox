#pragma once

#include "Rect.h"

#include <vector>

namespace phys {

// struct Body {
//     std::vector< Vec2 > positions;
//     std::vector< Vec2 > normals;
// };

struct CollisionInfo {
    int resolution[ 2 ];
};

bool collideAABB( const Rect & a, const Rect & b, CollisionInfo & info );

// bool collide( const Body & a, const Body & b, CollisionInfo & info );

} // namespace phys
