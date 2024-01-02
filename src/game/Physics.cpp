#include "Physics.h"

namespace phys {

struct Interval {
    int a;
    int b;
};

/// @returns how much `first` would need to shift to not collide with `second`
static bool resolve( const Interval & first, const Interval & second,
                     int & resolution ) {
    if ( first.a >= second.a && first.a <= second.b ) {
        if ( first.b > second.b ) {
            resolution = second.b - first.a + 1;
        } else {
            // first is inside second
            resolution = 0;
        }
        return true;
    }

    if ( second.a >= first.a && second.a <= first.b ) {
        if ( second.b > first.b ) {
            resolution = -( first.b - second.a + 1 );
        } else {
            // second is inside first
            resolution = 0;
        }
        return true;
    }

    resolution = 0;

    return false;
}

bool collideAABB( const Rect & a, const Rect & b, CollisionInfo & info ) {

    const Interval aIntervalX = { a.x, a.x + a.w - 1 };
    const Interval aIntervalY = { a.y, a.y + a.h - 1 };
    const Interval bIntervalX = { b.x, b.x + b.w - 1 };
    const Interval bIntervalY = { b.y, b.y + b.h - 1 };

    int resolveX;
    int resolveY;

    const bool collidedX = resolve( aIntervalX, bIntervalX, resolveX );
    const bool collidedY = resolve( aIntervalY, bIntervalY, resolveY );

    info.resolution[ 0 ] = resolveX;
    info.resolution[ 1 ] = resolveY;

    return collidedX && collidedY;
}

} // namespace phys
