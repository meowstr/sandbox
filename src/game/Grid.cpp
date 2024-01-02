#include "Grid.h"

namespace grid {

size_t size( const Info & info ) {
    return info.width * info.height;
}

grid::Coord coord( const Info & info, size_t index ) {
    grid::Coord c;

    c.x = index % info.width;
    c.y = index / info.width;

    return c;
}

size_t index( const Info & info, Coord coord ) {
    return index( info, coord.x, coord.y );
}

size_t index( const Info & info, Size x, Size y ) {
    return x + y * info.width;
}

bool contains( const Info & info, Coord coord ) {
    return contains( info, coord.x, coord.y );
}

bool contains( const Info & info, Size x, Size y ) {
    return x >= 0 && y >= 0 && x < info.width && y < info.height;
}

} // namespace grid
