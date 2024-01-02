#include "GridAstar.h"

#include "Logging.h"

#include <algorithm>
#include <cmath>

namespace astar {

static const std::vector< grid::Coord > kNborOffsets = {
    grid::Coord{ 1, 0 },  grid::Coord{ -1, 0 },  grid::Coord{ 0, 1 },
    grid::Coord{ 0, -1 }, grid::Coord{ 1, 1 },   grid::Coord{ -1, 1 },
    grid::Coord{ 1, -1 }, grid::Coord{ -1, -1 },
};
static const std::vector< int > kNborWeights = { 10, 10, 10, 10,
                                                 14, 14, 14, 14 };

int randCost( grid::Coord node ) {
    int rx = (int) ( 6.0f * sin( 0.1f + node.x * 2.3f ) );
    int ry = (int) ( 6.0f * cos( 0.3f + node.y * 2.3f ) );
    int r = rx + ry;
    return r;
}

int octileHeuristic( grid::Coord node, grid::Coord end ) {
    int D = 10;
    int D2 = 14;
    int dx = std::abs( node.x - end.x );
    int dy = std::abs( node.y - end.y );
    return D * ( dx + dy ) + ( D2 - 2 * D ) * std::min( dx, dy );
}

int manhattanHeuristic( grid::Coord node, grid::Coord end ) {
    int dx = std::abs( node.x - end.x );
    int dy = std::abs( node.y - end.y );
    return ( dx + dy ) * 10;
}

int heuristic( grid::Coord node, grid::Coord end ) {
    return octileHeuristic( node, end );
}

Path shortestPath( grid::Info gridInfo, const std::vector< int > & gridData,
                   grid::Coord start, grid::Coord end, int iterationMax ) {

    // swap the start and end so we don't need to reverse the path on
    // reconstruction
    // std::swap( start, end );

    size_t startIndex = grid::index( gridInfo, start );
    size_t endIndex = grid::index( gridInfo, end );
    size_t gridSize = gridInfo.width * gridInfo.height;

    // DEBUG_LOG() << "grid size: " << gridSize << std::endl;

    std::vector< int > g( gridSize, 0 );
    std::vector< size_t > parent( gridSize, gridSize );
    std::vector< size_t > open;
    std::vector< grid::Coord > coord( gridSize );

    for ( size_t i = 0; i < gridSize; i++ ) {
        grid::Coord c;
        c.x = i % gridInfo.width;
        c.y = i / gridInfo.width;
        coord[ i ] = c;

        // if ( grid::index( gridInfo, c ) != i ) {
        //     ERROR_LOG() << "AHHHH" << std::endl;
        // }
    }

    open.push_back( grid::index( gridInfo, start ) );

    int watchdog1 = 0;
    size_t bestCell;
    while ( !open.empty() ) {
        // DEBUG_LOG() << "iterate astar" << std::endl;

        // find best cell in open
        size_t bestCellOpenIndex = 0;
        bestCell = open[ bestCellOpenIndex ];
        int bestCellScore = g[ bestCell ] + heuristic( coord[ bestCell ], end );

        for ( size_t i = 1; i < std::ssize( open ); i++ ) {
            size_t cell = open[ i ];
            int gScore = g[ cell ];
            int hScore = heuristic( coord[ cell ], end );
            int score = gScore + hScore;

            // DEBUG_LOG() << "open cell: " << coord[ cell ].x << ", "
            //             << coord[ cell ].y << " g:" << gScore << " h:" <<
            //             hScore
            //             << std::endl;

            if ( score <= bestCellScore ) {
                bestCellOpenIndex = i;
                bestCell = cell;
                bestCellScore = score;
            }
        }

        // DEBUG_LOG() << "best open cell: " << coord[ bestCell ].x << ", "
        //             << coord[ bestCell ].y << " g:" << g[ bestCell ]
        //             << " h:" << heuristic( coord[ bestCell ], end )
        //             << std::endl;

        watchdog1++;
        if ( watchdog1 >= iterationMax ) {
            // find the best heuristic cell

            bestCell = open[ 0 ];
            for ( size_t i : open ) {
                if ( heuristic( coord[ i ], end ) <
                     heuristic( coord[ bestCell ], end ) ) {
                    bestCell = i;
                }
            }

            break;
        }

        open.erase( open.begin() + bestCellOpenIndex );

        // found the end
        if ( bestCell == endIndex ) {
            break;
        }

        // look at neighbors
        for ( size_t i = 0; i < 8; i++ ) {
            grid::Coord offset = kNborOffsets[ i ];
            int weight = kNborWeights[ i ];

            grid::Coord nborCoord;
            nborCoord.x = coord[ bestCell ].x + offset.x;
            nborCoord.y = coord[ bestCell ].y + offset.y;

            // weight += randCost( nborCoord );

            // out of bounds
            if ( !grid::contains( gridInfo, nborCoord ) ) {
                continue;
            }

            size_t nbor = grid::index( gridInfo, nborCoord );

            // ignore start cell (don't want to overwrite parent)
            if ( nbor == startIndex ) {
                continue;
            }

            // skip non solids
            if ( gridData[ nbor ] != 0 ) {
                continue;
            }

            int newG = g[ bestCell ] + weight;

            if ( parent[ nbor ] == gridSize ) {
                // unvisited
                g[ nbor ] = newG;
                parent[ nbor ] = bestCell;
                open.push_back( nbor );
            } else {
                // visited

                if ( newG < g[ nbor ] ) {
                    auto oldCoord = coord[ parent[ nbor ] ];

                    // DEBUG_LOG() << "cell re-evaluated" << std::endl;
                    // DEBUG_LOG() << "cell pos " << nborCoord.x << ", "
                    //             << nborCoord.y << std::endl;
                    // DEBUG_LOG() << "old g " << g[ nbor ] << std::endl;
                    // DEBUG_LOG() << "old parent " << oldCoord.x << ", "
                    //             << oldCoord.y << std::endl;
                    g[ nbor ] = newG;
                    parent[ nbor ] = bestCell;

                    auto newCoord = coord[ parent[ nbor ] ];
                    // DEBUG_LOG() << "new g " << g[ nbor ] << std::endl;
                    // DEBUG_LOG() << "new parent " << newCoord.x << ", "
                    //             << newCoord.y << std::endl;

                    // add to open list if not already
                    if ( std::find( open.begin(), open.end(), nbor ) ==
                         open.end() ) {
                        ERROR_LOG() << "closed node re-evaluated!" << std::endl;
                        open.push_back( nbor );
                    }
                }
            }
        }
    }

    //DEBUG_LOG() << "A* took " << watchdog1 << " iterations" << std::endl;

    // build path
    Path path;

    for ( size_t i : open ) {
        path.debugOpenPoints.push_back( coord[ i ] );
    }

    // return empty path
    // if ( parent[ endIndex ] == gridSize )
    //    return path;

    // size_t cell = endIndex;
    size_t cell = bestCell;

    int watchdog2;

    const int segMaxLength = 10;
    int segCounter = 0;

    while ( cell != gridSize ) {
        // DEBUG_LOG() << "iterate reconstruct" << std::endl;

        watchdog2++;
        if ( watchdog2 >= 100000 ) {
            return Path{};
        }

        grid::Coord c = coord[ cell ];
        cell = parent[ cell ];

        // overwrite points that are colinear
        if ( path.points.size() >= 2 && segCounter < segMaxLength ) {
            grid::Coord & c1 = path.points[ path.points.size() - 2 ];
            grid::Coord & c2 = path.points[ path.points.size() - 1 ];

            int dx1 = c2.x - c1.x;
            int dy1 = c2.y - c1.y;

            int dx2 = c.x - c2.x;
            int dy2 = c.y - c2.y;

            if ( dx1 * dy2 == dx2 * dy1 ) {
                c2 = c;
                segCounter++;
            } else {
                segCounter = 0;
                path.points.push_back( c );
            }
        } else {
            segCounter = 0;
            path.points.push_back( c );
        }
    }

    std::reverse( path.points.begin(), path.points.end() );

    return path;
}

} // namespace astar
