#include "Math.h"
#include "Test.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <stdexcept>

namespace math {

////////////////////////////////////////////////////////////////////////////////

bool pointInsideTriangle2D( glm::vec2 t1, glm::vec2 t2, glm::vec2 t3,
                            glm::vec2 p ) {

    glm::vec2 v1 = t2 - t1;
    glm::vec2 v2 = t3 - t1;

    // tri space to world space represented by
    // [ v1 v2 ] matrix

    float det = v1.x * v2.y - v2.x * v1.y;

    // TODO: remove check for speed
    if ( det == 0.0f ) {
        // throw std::runtime_error( "pointInsideTriangle2D: invalid triangle"
        // );
        // no way point p is exactly on this line/point
        return false;
    }

    float invdet = std::abs( 1.0f / det );

    // columns of inverse
    glm::vec2 c1 = { invdet * v2.y, invdet * -v1.y };
    glm::vec2 c2 = { invdet * -v2.x, invdet * v1.x };

    // p in triangle space
    glm::vec2 q = p.x * c1 + p.y * c2;

    // check whether p is in triangle, in triangle space
    return q.x >= 0.0f && q.y >= 0.0f && q.x + q.y <= 1.0f;
}

////////////////////////////////////////////////////////////////////////////////

glm::vec2 closestPointOnSegment2D( glm::vec2 s1, glm::vec2 s2, glm::vec2 p ) {
    glm::vec2 v = s2 - s1;
    glm::vec2 u = p - s1;
    // project u onto v
    float t = glm::clamp( glm::dot( u, v ) / glm::length2( v ), 0.0f, 1.0f );
    return s1 + v * t;
}

////////////////////////////////////////////////////////////////////////////////

Segments segmentsFromTriangles2D( const std::vector< glm::vec2 > & vertices ) {
    const float distanceTolerance = 1e-5f;
    const float distanceTolerance2 = distanceTolerance * distanceTolerance;

    struct SegmentCount {
        Segment segment;
        int count;
    };

    std::vector< SegmentCount > segmentCounts;

    auto addSegment = [ & ]( Segment segment ) {
        for ( SegmentCount & segmentCount : segmentCounts ) {
            // check if segment is close enough to an existing one (given order
            // and flipped order)
            if ( glm::distance2( segment.first, segmentCount.segment.first ) <
                     distanceTolerance2 &&
                 glm::distance2( segment.second, segmentCount.segment.second ) <
                     distanceTolerance2 ) {
                segmentCount.count++;
                return;
            } else if ( glm::distance2( segment.second,
                                        segmentCount.segment.first ) <
                            distanceTolerance2 &&
                        glm::distance2( segment.first,
                                        segmentCount.segment.second ) <
                            distanceTolerance2 ) {
                segmentCount.count++;
                return;
            }
        }
        segmentCounts.push_back( { segment, 1 } );
    };

    LOGGER_ASSERT( vertices.size() % 3 == 0 );
    for ( size_t i = 0; i < vertices.size(); i += 3 ) {
        addSegment( Segment{ vertices[ i + 0 ], vertices[ i + 1 ] } );
        addSegment( Segment{ vertices[ i + 1 ], vertices[ i + 2 ] } );
        addSegment( Segment{ vertices[ i + 2 ], vertices[ i + 0 ] } );
    }

    // categorize by segment count
    Segments segments;
    for ( SegmentCount & segmentCount : segmentCounts ) {
        if ( segmentCount.count > 1 ) {
            // duplicate entries must mean that multiple triangles share this
            // segment, thus it is internal
            segments.internal.push_back( segmentCount.segment );
        } else {
            // if it's not internal then it's a boundary segment
            segments.boundary.push_back( segmentCount.segment );
        }
    }

    return segments;
}

////////////////////////////////////////////////////////////////////////////////

QuadData extractQuadData( const std::vector< glm::vec3 > & vertices ) {

    LOGGER_ASSERT( vertices.size() == 6 );

    QuadData data;

    std::vector< glm::vec3 > duplicates;
    std::vector< glm::vec3 > nonduplicates;

    auto closeVec3 = []( glm::vec3 a, glm::vec3 b ) {
        return glm::distance2( a, b ) < 0.0001;
    };

    for ( glm::vec3 v : vertices ) {
        auto it = std::find_if(
            nonduplicates.begin(), nonduplicates.end(),
            [ v, closeVec3 ]( auto other ) { return closeVec3( v, other ); } );
        if ( it != nonduplicates.end() ) {
            duplicates.push_back( v );
            // remove from nonduplicate
            nonduplicates.erase( it );
        } else {
            nonduplicates.push_back( v );
        }
    }

    LOGGER_ASSERT( duplicates.size() == 2 );
    LOGGER_ASSERT( nonduplicates.size() == 2 );

    // find a vertex not on the diagonal segment
    glm::vec3 goodVertex = nonduplicates[ 0 ];

    // compute two edges
    glm::vec3 edge1 = duplicates[ 0 ] - goodVertex;
    glm::vec3 edge2 = duplicates[ 1 ] - goodVertex;

    // choose the edge that aligns best with y-axis as the up vector
    float alignment1 = glm::dot( edge1, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    float alignment2 = glm::dot( edge1, glm::vec3( 0.0f, 1.0f, 0.0f ) );

    if ( std::abs( alignment2 ) > std::abs( alignment1 ) ) {
        std::swap( edge1, edge2 );
        std::swap( alignment1, alignment2 );
    }

    // edge1 is the up vector (correct sign so that it is positive aligned)
    data.up = edge1 * ( ( alignment1 < 0.0f ) ? -1.0f : 1.0f );
    data.height = glm::length( edge1 );
    data.width = glm::length( edge2 );

    // compute center as average of 4 vertices
    glm::vec3 total = nonduplicates[ 0 ] + nonduplicates[ 1 ] +
                      duplicates[ 0 ] + duplicates[ 1 ];
    data.center = total * 0.25f;

    return data;
}

////////////////////////////////////////////////////////////////////////////////

static void testPointInsideTriangle2D() {
    LOGGER_ASSERT( pointInsideTriangle2D( { 0.0, 0.0 }, { 2.0, 0.0 },
                                          { 0.0, 2.0 }, { 0.9, 1.0 } ) );

    LOGGER_ASSERT( !pointInsideTriangle2D( { 0.0, 0.0 }, { 2.0, 0.0 },
                                           { 0.0, 2.0 }, { 1.1, 1.0 } ) );
}

////////////////////////////////////////////////////////////////////////////////

static void testSegmentsFromTriangles2D() {
    std::vector< glm::vec2 > v = {
        { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f },
        { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f },
    };

    Segments segments = segmentsFromTriangles2D( v );

    LOGGER_ASSERT( segments.boundary.size() == 4 );
    LOGGER_ASSERT( segments.internal.size() == 1 );

    DEBUG_LOG() << "Boundary:" << std::endl;
    for ( auto s : segments.boundary ) {
        DEBUG_LOG() << s.first.x << " " << s.first.y << " " << s.second.x << " "
                    << s.second.y << std::endl;
    }
    DEBUG_LOG() << "Internal:" << std::endl;
    for ( auto s : segments.internal ) {
        DEBUG_LOG() << s.first.x << " " << s.first.y << " " << s.second.x << " "
                    << s.second.y << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////

Rect marginRect( Rect rect, int margin ) {
    rect.x += margin;
    rect.y += margin;
    rect.w -= 2 * margin;
    rect.h -= 2 * margin;
    return rect;
}

////////////////////////////////////////////////////////////////////////////////

bool contains( Rect rect, int x, int y ) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
           y < rect.y + rect.h;
}

////////////////////////////////////////////////////////////////////////////////

Poly3 hermite( float p1, float p2, float v1, float v2 ) {
    Poly3 p;

    p.k[ 0 ] = p1;
    p.k[ 1 ] = v1;
    p.k[ 2 ] = ( -3 * p1 ) + ( -2 * v1 ) + ( -v2 ) + ( 3 * p2 );
    p.k[ 3 ] = ( 2 * p1 ) + ( v1 ) + ( v2 ) + ( -2 * p2 );

    return p;
}

////////////////////////////////////////////////////////////////////////////////

float eval( Poly3 p, float t ) {
    float t2 = t * t;
    float t3 = t2 * t;
    return p.k[ 0 ] + p.k[ 1 ] * t + p.k[ 2 ] * t2 + p.k[ 3 ] * t3;
}

////////////////////////////////////////////////////////////////////////////////

std::vector< glm::vec2 > spline( const std::vector< glm::vec2 > & in,
                                 float ds ) {
    const float tension = 1.0f;
    const float ds2 = ds * ds;

    std::vector< glm::vec2 > out;

    if ( in.empty() ) {
        return out;
    }

    out.push_back( in[ 0 ] );

    for ( int i = 1; i < std::ssize( in ); i++ ) {
        int i1 = i - 1;
        int i2 = i;

        glm::vec2 p1 = in[ i1 ];
        glm::vec2 p2 = in[ i2 ];

        glm::vec2 v1;
        glm::vec2 v2;

        if ( i1 >= 1 && i1 < std::ssize( in ) - 1 ) {
            v1 = in[ i1 + 1 ] - in[ i1 - 1 ];
        } else {
            v1 = glm::vec2{ 0.0f, 1.0f };
        }

        if ( i2 >= 1 && i2 < std::ssize( in ) - 1 ) {
            v2 = in[ i2 + 1 ] - in[ i2 - 1 ];
        } else {
            v2 = glm::vec2{ 0.0f, 1.0f };
        }

        v1 = v1 / ( tension );
        v2 = v2 / ( tension );

        Poly3 polyX = hermite( p1.x, p2.x, v1.x, v2.x );
        Poly3 polyY = hermite( p1.y, p2.y, v1.y, v2.y );

        auto eval = [ & ]( float t ) {
            return glm::vec2{ math::eval( polyX, t ), math::eval( polyY, t ) };
        };

        // sample curve with ds distance intervals
        glm::vec2 last = out.back();
        float t1 = 0.0f;

        while ( std::abs( t1 - 1.0f ) > 0.01f ) {
            // find t that is closer than ds
            float t2 = 1.0f;

            int watchdog1 = 100;
            while ( glm::distance2( eval( t1 ), eval( t2 ) ) > ds2 &&
                    watchdog1 > 0 ) {
                watchdog1--;
                // pick mid point of t1 and t2
                t2 = 0.5f * ( t1 + t2 );
            }

            out.push_back( eval( t2 ) );

            t1 = t2;
        }
    }

    return out;
}

////////////////////////////////////////////////////////////////////////////////

void runTests() {
    testPointInsideTriangle2D();
    testSegmentsFromTriangles2D();
}

} // namespace math
