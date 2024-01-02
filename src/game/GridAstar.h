#pragma once

#include "Grid.h"

#include <vector>

namespace astar {

struct Path {
    std::vector< grid::Coord > points;
    std::vector< grid::Coord > debugOpenPoints;
};

Path shortestPath( grid::Info gridInfo, const std::vector< int > & gridData,
                   grid::Coord start, grid::Coord end, int iterationMax );

} // namespace astar
