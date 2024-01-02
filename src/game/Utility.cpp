#include "Utility.h"

//#include "Map.h"

#include <fstream>

namespace utils {

//MapRaii::MapRaii( char * data ) {
//    map = readMap( data );
//}
//
//MapRaii::~MapRaii() {
//    freeMap( map );
//}

std::string stringFromFile( std::filesystem::path path ) {
    path.make_preferred();
    //  load file into string
    std::ifstream file( path );

    if ( !file ) {
        throw std::runtime_error( "Could not find file: " + path.string() );
    }

    std::string source( ( std::istreambuf_iterator< char >( file ) ),
                        ( std::istreambuf_iterator< char >() ) );

    return source;
}

} // namespace utils
