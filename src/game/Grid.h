#pragma once

#include <stddef.h>

namespace grid {
using Size = int;

struct Info {

    Size width;
    Size height;
};

struct Coord {
    Size x;
    Size y;
};

size_t size( const Info & info );

grid::Coord coord( const Info & info, size_t index );

size_t index( const Info & info, Coord coord );

size_t index( const Info & info, Size x, Size y );

bool contains( const Info & info, Coord coord );

bool contains( const Info & info, Size x, Size y );

} // namespace grid
