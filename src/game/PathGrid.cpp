#include "PathGrid.h"

#include <limits>

namespace pathGrid {

void iterate( Info info, grid::Coord coord ) {
    size_t index = grid::index( info.gridInfo, coord );

    // skip things that are masked out
    if ( ( *info.mask )[ index ] )
        return;

    // choose the lowest cost
    grid::Coord ns[ 4 ];

    ns[ 0 ] = coord;
    ns[ 1 ] = coord;
    ns[ 2 ] = coord;
    ns[ 3 ] = coord;

    ns[ 0 ].x += 1;
    ns[ 1 ].x -= 1;
    ns[ 2 ].y += 1;
    ns[ 3 ].y -= 1;

    int intMax = std::numeric_limits< int >::max();
    int cheapestCost = intMax;
    unsigned char cheapMask = 0;
    for ( int i = 0; i < 4; i++ ) {
        grid::Coord n = ns[ i ];
        if ( !grid::contains( info.gridInfo, n ) )
            continue;

        size_t ni = grid::index( info.gridInfo, n );

        int cost = ( *info.costs )[ ni ];

        if ( cost <= cheapestCost ) {
            if ( cost != cheapestCost ) {
                cheapMask = 0;
            }

            cheapestCost = cost;
            cheapMask |= 1 << ( 3 - i );
        }
    }

    // prevent overflow
    if ( cheapestCost < intMax ) {
        ( *info.costs )[ index ] = cheapestCost + 1;
    } else {
        ( *info.costs )[ index ] = intMax;
    }

    ( *info.field )[ index ] = cheapMask;
}

void iterateField( Info info ) {}

} // namespace pathGrid
