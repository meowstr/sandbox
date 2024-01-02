#include "NavMesh.h"

#include <glm/gtx/norm.hpp>

namespace engine {

namespace {
void resolvePoint( glm::vec2 & p, const NavMesh & navMesh ) {
    auto & firstSeg = navMesh.boundary.at( 0 );
    glm::vec2 closest =
        math::closestPointOnSegment2D( firstSeg.first, firstSeg.second, p );
    float closestDist = glm::distance2( p, closest );

    for ( size_t i = 1; i < navMesh.boundary.size(); i++ ) {
        auto & s = navMesh.boundary[ i ];

        glm::vec2 proj = math::closestPointOnSegment2D( s.first, s.second, p );
        float dist = glm::distance2( p, closest );

        if ( dist < closestDist ) {
            closest = proj;
            closestDist = dist;
        }
    }

    p = closest;
}

bool onNavMesh( const glm::vec2 & pos, const NavMesh & navMesh ) {
    for ( size_t i = 0; i < navMesh.vertices.size(); i += 3 ) {
        if ( math::pointInsideTriangle2D( navMesh.vertices[ i + 0 ],
                                          navMesh.vertices[ i + 1 ],
                                          navMesh.vertices[ i + 2 ], pos ) ) {
            return true;
        }
    }
    return false;
}

} // namespace

void moveOnNavMesh( glm::vec2 & pos, glm::vec2 delta,
                    const NavMesh & navMesh ) {
    pos += delta;
    if ( !onNavMesh( pos, navMesh ) ) {
        resolvePoint( pos, navMesh );
    }
}

} // namespace engine
