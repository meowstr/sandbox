#include "GameMap.h"

#include <map.h>

namespace map {

////////////////////////////////////////////////////////////////////////////////

Map::Map( std::filesystem::path path ) {
    map_t * map = load_map( path.c_str() );

    for ( int i = 0; i < map->prop_count; i++ ) {
        //std::cout << "[" << map->values[ i ] << "]" << std::endl;
        mMap[ map->names[ i ] ] = map->values[ i ];
    }

    unload_map( map );
}

////////////////////////////////////////////////////////////////////////////////

std::optional< std::string > Map::query( std::string key ) const {
    auto it = mMap.find( key );
    if ( it == mMap.end() ) {
        return std::nullopt;
    } else {
        return it->second;
    }
}

////////////////////////////////////////////////////////////////////////////////

void Map::put( std::string key, std::string value ) {
    mMap[ key ] = value;
}

////////////////////////////////////////////////////////////////////////////////

void Map::write( std::filesystem::path path ) {
    std::vector< const char * > names;
    std::vector< const char * > values;

    for ( auto && [ name, value ] : mMap ) {
        names.push_back( name.c_str() );
        values.push_back( value.c_str() );
    }

    map_t map;

    map.prop_count = mMap.size();
    map.names = names.data();
    map.values = values.data();

    write_map( &map, path.c_str() );
}

} // namespace map
