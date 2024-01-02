#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace engine {

struct Mesh;

namespace objReader {

struct ObjFileProperties {
    using Obj = std::pair< std::string, std::vector< size_t > >;
    std::vector< Obj > objs;
};

void loadMesh( Mesh & mesh, std::filesystem::path objPath,
               ObjFileProperties * oProps = nullptr );

void queryObjMesh( Mesh & oMesh, const Mesh & mesh,
                   const ObjFileProperties & props, const std::string & name );

} // namespace objReader

} // namespace engine
