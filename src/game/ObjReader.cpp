#include "ObjReader.h"

#include "Mesh.h"

#include "Logging.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Position {
    float x;
    float y;
    float z;

    operator glm::vec3() {
        return { x, y, z };
    }
};

struct TextureCoord {
    float u;
    float v;
    operator glm::vec2() {
        return { u, v };
    }
};

using Normal = Position;

struct Vertex {
    std::size_t positionIndex;
    std::size_t textureIndex;
    std::size_t normalIndex;
};

struct Face {
    std::vector< Vertex > vertices;
    size_t objIndex;
};

} // namespace

namespace engine::objReader {

void loadMesh( engine::Mesh & mesh, std::filesystem::path objPath,
               ObjFileProperties * oProps ) {
    mesh.vertices.clear();
    mesh.pos = glm::vec3{};
    mesh.size = glm::vec3{};

    INFO_LOG() << "Loading model " << objPath.generic_string() << "."
               << std::endl;
    std::ifstream objStream( objPath );

    std::vector< Position > positions;
    std::vector< TextureCoord > textureCoords;
    std::vector< Normal > normals;
    std::vector< Face > faces;
    ObjFileProperties props;

    // add default obj
    ObjFileProperties::Obj obj;
    obj.first = "";
    props.objs.push_back( obj );

    std::string currentObjName;

    if ( !objStream ) {
        throw std::runtime_error( "Could not find model (.obj) file: " +
                                  objPath.string() );
    }

    // parse the file
    while ( !objStream.eof() ) {
        std::string line;
        std::getline( objStream, line );
        std::stringstream lineStream( line );

        std::string operand;
        lineStream >> operand;

        if ( operand == "v" ) {
            // parse a position
            Position p;
            lineStream >> p.x;
            lineStream >> p.y;
            lineStream >> p.z;
            positions.push_back( p );
        } else if ( operand == "vt" ) {
            // parse a texture coord
            TextureCoord t;
            lineStream >> t.u;
            lineStream >> t.v;
            textureCoords.push_back( t );
        } else if ( operand == "vn" ) {
            // parse a normal
            Normal n;
            lineStream >> n.x;
            lineStream >> n.y;
            lineStream >> n.z;
            normals.push_back( n );
        } else if ( operand == "f" ) {
            // parse a face
            Face face;
            std::string vertexString;
            while ( lineStream >> vertexString ) {
                // parse a vertex
                std::stringstream vertexStream( vertexString );
                // parse vertex string as v/vt/vn
                ::Vertex vertex;
                for ( auto attrib :
                      { &::Vertex::positionIndex, &::Vertex::textureIndex,
                        &::Vertex::normalIndex } ) {
                    // parse an attribute
                    std::string attribString;
                    std::getline( vertexStream, attribString, '/' );
                    // tranform 1-indexed to 0-indexed
                    vertex.*attrib = std::stoi( attribString ) - 1;
                }
                face.vertices.push_back( vertex );
            }

            face.objIndex = props.objs.size() - 1;
            faces.push_back( face );
        } else if ( operand == "o" ) {
            lineStream >> currentObjName;
            ObjFileProperties::Obj obj;
            obj.first = currentObjName;
            props.objs.push_back( obj );
        }
    }

    if ( positions.empty() )
        return;

    glm::vec3 minPosition = positions[ 0 ];
    glm::vec3 maxPosition = positions[ 0 ];

    for ( Face & face : faces ) {
        // split into triangles
        for ( int i = 2; i < face.vertices.size(); i++ ) {

            int triIndices[ 3 ];

            // do a triangle fan
            triIndices[ 0 ] = 0;
            triIndices[ 1 ] = i - 1;
            triIndices[ 2 ] = i;

            // make new triangle
            for ( int j : triIndices ) {
                ::Vertex v = face.vertices[ j ];

                if ( v.positionIndex >= positions.size() ) {
                    throw std::runtime_error(
                        "Position index out of bounds: " +
                        std::to_string( v.positionIndex ) );
                }

                if ( v.normalIndex >= normals.size() ) {
                    throw std::runtime_error( "Normal index out of bounds: " +
                                              std::to_string( v.normalIndex ) );
                }

                if ( v.textureIndex >= textureCoords.size() ) {
                    throw std::runtime_error(
                        "Texture UV index out of bounds: " +
                        std::to_string( v.textureIndex ) );
                }

                engine::Vertex meshVertex = {
                    .position = positions[ v.positionIndex ],
                    .normal = normals[ v.normalIndex ],
                    .uv = textureCoords[ v.textureIndex ],
                };

                props.objs[ face.objIndex ].second.push_back(
                    mesh.vertices.size() );
                mesh.vertices.push_back( meshVertex );

                minPosition.x =
                    std::min( meshVertex.position.x, minPosition.x );
                minPosition.y =
                    std::min( meshVertex.position.y, minPosition.y );
                minPosition.z =
                    std::min( meshVertex.position.z, minPosition.z );

                maxPosition.x =
                    std::max( meshVertex.position.x, maxPosition.x );
                maxPosition.y =
                    std::max( meshVertex.position.y, maxPosition.y );
                maxPosition.z =
                    std::max( meshVertex.position.z, maxPosition.z );
            }
        }
    }

    mesh.size = maxPosition - minPosition;
    mesh.pos = minPosition;

    if ( oProps ) {
        *oProps = std::move( props );
    }
}

void queryObjMesh( Mesh & oMesh, const Mesh & mesh,
                   const ObjFileProperties & props, const std::string & name ) {

    auto it = std::find_if(
        props.objs.begin(), props.objs.end(),
        [ & ]( const auto & entry ) { return entry.first == name; } );

    if ( it == props.objs.end() ) {
        throw std::runtime_error( "No obj in objfile named: " + name + "." );
    }

    oMesh.vertices.clear();
    oMesh.pos = mesh.pos;
    oMesh.size = mesh.size;

    for ( size_t i : it->second ) {
        oMesh.vertices.push_back( mesh.vertices[ i ] );
    }
}

} // namespace engine::objReader
