#include "Shadows.h"

#include "Logging.h"
#include "Test.h"

#include <glm/gtx/norm.hpp>

namespace engine::graphics {

using Edge = std::pair< glm::vec3, glm::vec3 >;
using AdjFaces = std::pair< int, int >;

namespace {
bool closeEnough( const glm::vec3 & a, const glm::vec3 & b ) {
    return glm::distance2( a, b ) < 0.0001;
}

bool closeEnough( const Edge & a, const Edge & b ) {
    return ( closeEnough( a.first, b.first ) &&
             closeEnough( a.second, b.second ) ) ||
           ( closeEnough( a.first, b.second ) &&
             closeEnough( a.second, b.first ) );
}
} // namespace

void generateShadowMeshFromPoint( ShadowMesh & mesh,
                                  const std::vector< Vertex > & vertices,
                                  glm::vec3 pointLightPosition,
                                  float maxDistance ) {

    std::vector< std::pair< Edge, AdjFaces > > edgeMap;

    auto addEdge = [ & ]( Edge e, int face ) {
        for ( auto & entry : edgeMap ) {
            if ( closeEnough( entry.first, e ) ) {
                if ( entry.second.second == -1 ) {
                    entry.second.second = face;
                } else {
                    ERROR_LOG() << "Weird geometry." << std::endl;
                }
                return;
            }
        }

        std::pair< Edge, AdjFaces > newEntry{ e, { face, -1 } };
        edgeMap.push_back( newEntry );
    };

    for ( size_t i = 0; i < vertices.size(); i += 3 ) {
        Edge e1{ vertices[ i + 0 ].position, vertices[ i + 1 ].position };
        Edge e2{ vertices[ i + 1 ].position, vertices[ i + 2 ].position };
        Edge e3{ vertices[ i + 2 ].position, vertices[ i + 0 ].position };

        addEdge( e1, i );
        addEdge( e2, i );
        addEdge( e3, i );
    }

    std::vector< Edge > silEdges;

    for ( auto & entry : edgeMap ) {
        if ( entry.second.second ) {
            ERROR_LOG() << "Geometry contains cracks." << std::endl;
            continue;
        }

        glm::vec3 normal1 = vertices[ entry.second.first ].normal;
        glm::vec3 normal2 = vertices[ entry.second.second ].normal;

        glm::vec3 lightDir1 =
            vertices[ entry.second.first ].position - pointLightPosition;
        glm::vec3 lightDir2 =
            vertices[ entry.second.second ].position - pointLightPosition;

        if ( glm::dot( normal1, lightDir1 ) * glm::dot( normal2, lightDir2 ) <
             0 ) {
            // we have a silhoutte edge
            silEdges.push_back( entry.first );
        }
    }
}

} // namespace engine::graphics
