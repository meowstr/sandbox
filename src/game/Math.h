#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "Rect.h"

namespace math {

bool pointInsideTriangle2D( glm::vec2 t1, glm::vec2 t2, glm::vec2 t3,
                            glm::vec2 p );

glm::vec2 closestPointOnSegment2D( glm::vec2 s1, glm::vec2 s2, glm::vec2 p );

using Segment = std::pair< glm::vec2, glm::vec2 >;

struct Segments {
    std::vector< Segment > internal;
    std::vector< Segment > boundary;
};

Segments segmentsFromTriangles2D( const std::vector< glm::vec2 > & vertices );

// Ramer–Douglas–Peucker algorithm
std::vector< glm::vec2 > simplifyPath( const std::vector< glm::vec2 > & points,
                                       float e );

struct QuadData {
    glm::vec3 center;
    glm::vec3 up;
    float width;
    float height;
};

QuadData extractQuadData( const std::vector< glm::vec3 > & vertices );

Rect marginRect( Rect rect, int margin );

bool contains( Rect rect, int x, int y );

void runTests();

struct Poly3 {
    float k[ 4 ];
};

Poly3 hermite( float p1, float p2, float v1, float v2 );

float eval( Poly3 p, float t );

std::vector< glm::vec2 > spline( const std::vector< glm::vec2 > & points,
                                 float ds );

} // namespace math
