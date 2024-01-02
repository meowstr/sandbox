#pragma once

#include "Grid.h"

#include <vector>

namespace pathGrid {

using Vector = unsigned char;

struct Info {
    grid::Info gridInfo;
    std::vector< int > * mask;
    std::vector< int > * costs;
    std::vector< Vector > * field;

    size_t iterationIndex;
};

void iterate( Info info, grid::Coord coord );

void iterateChunk( Info info );

void iterateField( Info info );

} // namespace pathGrid
