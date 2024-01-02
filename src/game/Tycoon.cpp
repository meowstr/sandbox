#include "Tycoon.h"

#include "Graphics.h"
#include "GridAstar.h"
#include "Logging.h"
#include "Math.h"
#include "PathGrid.h"
#include "Physics.h"
#include "ResourceDirectory.h"
#include "ShaderProgram.h"
#include "Texture.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>

namespace tycoon {

static void tryReloadModule( state::GameState & state );
static void initModule( state::GameState & state );

static const std::vector< glm::vec4 > kColorPalette{
    glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f },
    glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f }, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f },
    glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f },
    glm::vec4{ 0.0f, 0.0f, 1.0f, 1.0f },
};

static const std::vector< glm::vec2 > kUnitQuadVertices = {
    glm::vec2{ -0.5f, -0.5f }, glm::vec2{ -0.5f, +0.5f },
    glm::vec2{ +0.5f, +0.5f }, glm::vec2{ -0.5f, -0.5f },
    glm::vec2{ +0.5f, +0.5f }, glm::vec2{ +0.5f, -0.5f },
};

static const std::vector< glm::vec2 > kTopLeftUnitQuadVertices = {
    glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 1.0f, 1.0f },
    glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 1.0f, 0.0f },
};

static const std::vector< glm::vec2 > kUnitQuadUvs = {
    glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 1.0f, 1.0f },
    glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f }, glm::vec2{ 1.0f, 0.0f },
};

static const std::vector< glm::vec2 > kArrowHeadVertices = {
    glm::vec2{ -0.5f, -0.3f }, glm::vec2{ -0.5f, +0.3f },
    glm::vec2{ +0.0f, +0.0f } };

static const std::vector< glm::vec2 > kArrowHeadUvs = {
    glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f }, glm::vec2{ 1.0f, 0.5f } };

static const std::vector< std::string > kToolNames = { "table", "wall", "human",
                                                       "eraser" };

// static const std::vector< state::KitchenType > kKitchenTypes{
//     state::KitchenType{ { 0, 1, 2 } },
//     state::KitchenType{ { 1, 2, 3 } },
// };

static const state::Recipes kRecipes{
    .durations = { 1, 2, 3, 4 },
    .prices = { 5, 15, 25, 30 },
};

////////////////////////////////////////////////////////////////////////////////

static glm::mat4 spriteTransform( int x, int y, int w, int h ) {
    glm::mat4 t( 1.0f );

    t = glm::translate( t, glm::vec3( x, y, 0.0f ) );
    t = glm::scale( t, glm::vec3( w, h, 1.0f ) );

    return t;
}

////////////////////////////////////////////////////////////////////////////////

static glm::mat4 spriteTransformTrunc( float x, float y, float w, float h ) {
    return spriteTransform( (int) x, (int) y, (int) w, (int) h );
}

////////////////////////////////////////////////////////////////////////////////

static glm::mat4 spriteTransform( const Rect & rect ) {
    return spriteTransform( rect.x, rect.y, rect.w, rect.h );
}

////////////////////////////////////////////////////////////////////////////////

static void drawSprite( state::GameState & state ) {
    gfx::draw( state.tycoon.topLeftUnitQuad );
}

static std::vector< grid::Coord >
toGridCoordVector( const std::vector< glm::vec2 > & v ) {
    std::vector< grid::Coord > out;

    for ( auto & x : v ) {
        out.push_back( grid::Coord{ (int) x.x, (int) x.y } );
    }

    return out;
}

static std::vector< glm::vec2 >
toVec2Vector( const std::vector< grid::Coord > & v ) {
    std::vector< glm::vec2 > out;

    for ( auto & x : v ) {
        out.push_back( glm::vec2{ (float) x.x, (float) x.y } );
    }

    return out;
}

////////////////////////////////////////////////////////////////////////////////

/// Transforms a unit quad (0,0)-(1,1) to a line
static glm::mat4 lineTransform( int x1, int x2, int y1, int y2,
                                int width = 1 ) {

    // this is fastest way I could think of...
    glm::vec2 pixelBias{ 0.5f, 0.5f };

    glm::vec2 x{ x1, x2 };
    glm::vec2 y{ y1, y2 };

    x += pixelBias;
    y += pixelBias;

    glm::vec2 dir = x - y;

    float hwidth = 0.5f * (float) width;
    float llen = glm::length( dir );

    glm::vec2 perp{ dir.y, -dir.x };
    glm::vec2 sperp = perp * hwidth / llen;

    glm::vec2 lenbias = dir * 0.5f / llen;

    glm::vec2 ss1 = x + sperp + lenbias;
    glm::vec2 ss2 = x - sperp + lenbias;
    glm::vec2 st1 = y + sperp - lenbias;
    glm::vec2 st2 = y - sperp - lenbias; // not needed

    glm::vec4 s1{ ss1.x, ss1.y, 0.0f, 1.0f };
    glm::vec4 s2{ ss2.x, ss2.y, 0.0f, 1.0f };
    glm::vec4 t1{ st1.x, st1.y, 0.0f, 1.0f };
    glm::vec4 t2{ st2.x, st2.y, 0.0f, 1.0f }; // not needed

    glm::mat4 t;

    t[ 0 ] = t1 - s1;
    t[ 1 ] = s2 - s1;
    t[ 2 ] = glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f }; // arbitrary
    t[ 3 ] = s1;

    return t;
}

struct LineAndArrow {
    glm::mat4 lineTransform;
    glm::mat4 arrowTransform;
};

////////////////////////////////////////////////////////////////////////////////

static LineAndArrow lineAndArrowTransform( int x1, int x2, int y1, int y2,
                                           int width = 1, int arrowSize = 15 ) {
    LineAndArrow out;

    glm::vec2 pixelBias{ 0.5f, 0.5f };

    glm::vec2 x{ x1, x2 };
    glm::vec2 y{ y1, y2 };

    x += pixelBias;
    y += pixelBias;

    glm::vec2 dir = y - x;

    float hwidth = 0.5f * (float) width;
    float llen = glm::length( dir );

    glm::vec2 perp{ dir.y, -dir.x };
    glm::vec2 sperp = perp * hwidth / llen;

    glm::vec2 ss1 = x + sperp;
    glm::vec2 ss2 = x - sperp;
    glm::vec2 st1 = y + sperp;
    glm::vec2 st2 = y - sperp; // not needed

    glm::vec4 s1{ ss1.x, ss1.y, 0.0f, 1.0f };
    glm::vec4 s2{ ss2.x, ss2.y, 0.0f, 1.0f };
    glm::vec4 t1{ st1.x, st1.y, 0.0f, 1.0f };
    glm::vec4 t2{ st2.x, st2.y, 0.0f, 1.0f }; // not needed

    glm::mat4 t;

    t[ 0 ] = t1 - s1;
    t[ 1 ] = s2 - s1;
    t[ 2 ] = glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f }; // arbitrary
    t[ 3 ] = s1;

    out.lineTransform = t;

    glm::vec2 arrowDir = dir * (float) arrowSize / llen;
    glm::vec2 arrowPerp = perp * (float) arrowSize / llen;

    t[ 0 ] = glm::vec4{ arrowDir.x, arrowDir.y, 0.0f, 0.0f };
    t[ 1 ] = glm::vec4{ arrowPerp.x, arrowPerp.y, 0.0f, 0.0f };
    t[ 2 ] = glm::vec4{ 0.0f, 0.0f, 1.0f, 0.0f }; // arbitrary
    t[ 3 ] = glm::vec4{ y1, y2, 0.0f, 1.0f };

    out.arrowTransform = t;

    return out;
}

////////////////////////////////////////////////////////////////////////////////

template < typename T >
static bool vectorFind( const std::vector< T > & v, const T & t,
                        int * index = nullptr ) {
    for ( int i = 0; i < std::ssize( v ); i++ ) {
        if ( v[ i ] == t ) {
            if ( index ) {
                *index = i;
            }
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

template < typename T >
void vectorRemoveByIndex( std::vector< T > & v, state::index_t index ) {
    if ( index == v.size() - 1 ) {
        v.pop_back();
    } else {
        T t = v.back();
        v.pop_back();
        v[ index ] = t;
    }
}

////////////////////////////////////////////////////////////////////////////////

static void drawLine( state::GameState & state ) {
    gfx::draw( state.tycoon.topLeftUnitQuad );
}

////////////////////////////////////////////////////////////////////////////////

static glm::vec2 screenToView( state::GameState & state, float x, float y ) {
    return { (int) ( x / state.rendering.pixelelizeFactor ),
             (int) ( y / state.rendering.pixelelizeFactor ) };
}

////////////////////////////////////////////////////////////////////////////////

static bool collidesWithWalls( state::GameState & state, const Rect & rect ) {

    for ( auto & wall : state.tycoon.walls ) {
        phys::CollisionInfo info;
        if ( phys::collideAABB( rect, wall, info ) ) {
            return true;
        }
    }

    return false;
};

////////////////////////////////////////////////////////////////////////////////

void updateConsoleText( state::GameState & state ) {
    std::stringstream ss;
    for ( std::string & str : state.tycoon.console.lines ) {
        ss << "> " << str << '\n';
    }

    gfx::writeTextBuffer( state.tycoon.consoleTextBuffer, state.tycoon.font,
                          ss.str(), glm::vec2{ 0.0f, 0.0f }, 1.0f );
}

////////////////////////////////////////////////////////////////////////////////

void addConsoleLine( state::GameState & state, const std::string & line ) {
    //
    const int maxLines = 5;

    auto & lines = state.tycoon.console.lines;

    if ( state.tycoon.console.timeLeft <= 0.0f ) {
        lines.clear();
    }

    if ( lines.size() < maxLines ) {
        lines.push_back( line );
    } else {
        // shift them out
        for ( int i = 0; i < maxLines - 1; i++ ) {
            lines[ i ] = lines[ i + 1 ];
        }
        lines[ maxLines - 1 ] = line;
    }

    state.tycoon.console.timeLeft = 2.0f;

    updateConsoleText( state );
}

////////////////////////////////////////////////////////////////////////////////

static void tickTimers( std::vector< float > & ioTimers, float timestep ) {
    for ( auto & timer : ioTimers ) {
        timer -= timestep;
        if ( timer <= 0.0f ) {
            timer = 0.0f;
        }
    }
}

#define GET_TABLE_COLUMN( table, out, fieldName )                              \
    for ( size_t i = 0; i < table.size(); i++ ) {                              \
        out[ i ] = table[ i ].fieldName;                                       \
    }

#define SET_TABLE_COLUMN( table, in, fieldName )                               \
    for ( size_t i = 0; i < table.size(); i++ ) {                              \
        table[ i ].fieldName = in[ i ];                                        \
    }

#define DEFINE_COLUMN( name, table, fieldName )                                \
    std::vector< decltype( table[ 0 ].fieldName ) > name( table.size() )

static void computeCollisionGrid( state::GameState & state ) {
    // build collision grid
    grid::Info & gridInfo = state.tycoon.collisionGridInfo;
    std::vector< int > & grid = state.tycoon.collisionGridData;
    std::vector< int > & pathGridCosts = state.tycoon.pathGridCosts;
    std::vector< unsigned char > & pathGridField = state.tycoon.pathGridField;

    gridInfo.width = state.rendering.subRenderWidth;
    gridInfo.height = state.rendering.subRenderHeight;

    size_t gridSize = gridInfo.width * gridInfo.height;

    int defaultCost = std::numeric_limits< int >::max();

    grid = std::vector< int >( gridSize, 0 );
    pathGridCosts = std::vector< int >( gridSize, defaultCost );
    pathGridField = std::vector< unsigned char >( gridSize, 0 );

    for ( Rect wall : state.tycoon.walls ) {
        wall = math::marginRect( wall, -1 );

        for ( int i = 0; i < wall.w; i++ ) {
            for ( int j = 0; j < wall.h; j++ ) {
                int x = wall.x + i;
                int y = wall.y + j;

                if ( grid::contains( gridInfo, x, y ) ) {
                    auto index = grid::index( gridInfo, x, y );
                    grid[ index ] = 1;
                }
            }
        }
    }

    grid::Coord pathGridTarget;
    pathGridTarget.x = gridInfo.width / 2;
    pathGridTarget.y = gridInfo.height / 2;

    size_t targetIndex = grid::index( gridInfo, pathGridTarget );
    pathGridCosts[ targetIndex ] = 0;
}

static void iteratePathGrids( state::GameState & state ) {
    pathGrid::Info info;
    info.gridInfo = state.tycoon.collisionGridInfo;
    info.mask = &state.tycoon.collisionGridData;
    info.costs = &state.tycoon.pathGridCosts;
    info.field = &state.tycoon.pathGridField;

    grid::Info gridInfo = info.gridInfo;

    grid::Coord pathGridTarget;
    pathGridTarget.x = gridInfo.width / 2;
    pathGridTarget.y = gridInfo.height / 2;

    size_t targetIndex = grid::index( gridInfo, pathGridTarget );

    static int startI = 0;
    startI = ( startI + 1 ) % 2;

    for ( size_t i = startI; i < grid::size( info.gridInfo ); i += 2 ) {

        if ( i == targetIndex )
            continue;

        grid::Coord coord = grid::coord( info.gridInfo, i );

        // coord.x = rand() % info.gridInfo.width;
        // coord.y = rand() % info.gridInfo.height;

        pathGrid::iterate( info, coord );
    }
}

static astar::Path * computePathForHuman( state::GameState & state,
                                          glm::vec2 pos, glm::vec2 target ) {

    grid::Info & gridInfo = state.tycoon.collisionGridInfo;
    std::vector< int > & grid = state.tycoon.collisionGridData;

    grid::Coord start;
    grid::Coord end;

    start.x = pos.x;
    start.y = pos.y;
    end.x = target.x;
    end.y = target.y;

    if ( !grid::contains( gridInfo, start.x, start.y ) ) {
        return nullptr;
    }

    if ( !grid::contains( gridInfo, end.x, end.y ) ) {
        return nullptr;
    }

    auto pathPtr = new astar::Path();
    *pathPtr = astar::shortestPath( gridInfo, grid, start, end, 100 );

    return pathPtr;
}

static state::id_t genId() {
    static state::id_t count = 0;
    return count++;
}

static void addHuman( state::GameState & state, glm::vec2 pos ) {
    state::TycoonSim & sim = state.tycoon.tycoonSim;

    glm::vec2 target{ state.rendering.subRenderWidth * 0.5f,
                      state.rendering.subRenderHeight * 0.5f };

    sim.customers.position.push_back( pos );
    sim.customers.velocity.push_back( glm::vec2{ 0, 0 } );
    sim.customers.subtarget.push_back( pos );
    sim.customers.target.push_back( target );
    sim.customers.path.push_back( nullptr );
    sim.customers.pathIndex.push_back( 0 );
    sim.customers.repathTimer.push_back( 0.0f );
    sim.customers.id.push_back( genId() );
}

static void removeHuman( state::GameState & state, state::id_t id ) {
    state::TycoonSim & sim = state.tycoon.tycoonSim;

    state::index_t index;
    if ( !vectorFind( sim.customers.id, id, &index ) ) {
        LOGGER_ASSERT( false );
    }

    // DEBUG_LOG() << "Removing human " << id << " from index " << index
    //             << std::endl;

    vectorRemoveByIndex( sim.customers.position, index );
    vectorRemoveByIndex( sim.customers.velocity, index );
    vectorRemoveByIndex( sim.customers.subtarget, index );
    vectorRemoveByIndex( sim.customers.target, index );
    vectorRemoveByIndex( sim.customers.path, index );
    vectorRemoveByIndex( sim.customers.pathIndex, index );
    vectorRemoveByIndex( sim.customers.repathTimer, index );
    vectorRemoveByIndex( sim.customers.id, index );
}

static bool validCollisionCell( state::GameState & state, grid::Coord coord ) {
    if ( grid::contains( state.tycoon.collisionGridInfo, coord ) ) {
        auto index = grid::index( state.tycoon.collisionGridInfo, coord );
        if ( state.tycoon.collisionGridData[ index ] == 0 ) {
            return true;
        }
    }
    return false;
}

static void tickPathables( state::GameState & state ) {
    state::TycoonSim & sim = state.tycoon.tycoonSim;

    using index_t = int;

    const float step = 20.0f * state.tickTimestep;
    const float deltaTime = state.tickTimestep;

    std::vector< glm::vec2 > & pos = sim.customers.position;
    std::vector< glm::vec2 > & vel = sim.customers.velocity;
    std::vector< glm::vec2 > & subtarget = sim.customers.subtarget;
    std::vector< glm::vec2 > & target = sim.customers.target;
    std::vector< astar::Path * > & path = sim.customers.path;
    std::vector< index_t > & pathIndex = sim.customers.pathIndex;
    std::vector< float > & repathTimer = sim.customers.repathTimer;
    std::vector< state::id_t > & stoppedAtTargetId = sim.customersAtTarget;
    std::vector< state::id_t > & id = sim.customers.id;

    std::vector< glm::vec2 > dir;
    std::vector< float > len;

    ////////////////////////////////////////////////////////////////////////////
    // compute dir and len
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        glm::vec2 localDir = subtarget[ index ] - pos[ index ];
        dir.push_back( localDir );
        len.push_back( glm::length( localDir ) );
    }

    std::vector< index_t > running;
    std::vector< index_t > stopped;

    ////////////////////////////////////////////////////////////////////////////
    // split up into running and stopped humans
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        if ( len[ index ] < step ) {
            stopped.push_back( index );
        } else {
            running.push_back( index );
        }
    }

    std::vector< index_t > stoppedAtSubtarget;
    std::vector< index_t > stoppedAtTarget;

    ////////////////////////////////////////////////////////////////////////////
    // split up into humans stopped at final targets and just subtargets
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stopped ) {
        if ( glm::distance2( target[ index ], subtarget[ index ] ) < 4.0 ) {
            stoppedAtTarget.push_back( index );
        } else {
            stoppedAtSubtarget.push_back( index );
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // move running forward
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : running ) {
        pos[ index ] += step * dir[ index ] / len[ index ];
    }

    ////////////////////////////////////////////////////////////////////////////
    // stop stopped
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stopped ) {
        pos[ index ] = subtarget[ index ];
    }

    std::vector< index_t > hasNextTarget;
    std::vector< index_t > noNextTarget;
    std::vector< index_t > noPath;

    ////////////////////////////////////////////////////////////////////////////
    // split up by whether they have a next target
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stoppedAtSubtarget ) {
        astar::Path * localPath = path[ index ];
        if ( !localPath ) {
            noPath.push_back( index );
        } else if ( pathIndex[ index ] + 1 < localPath->points.size() ) {
            hasNextTarget.push_back( index );
        } else {
            noNextTarget.push_back( index );
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // increment subtarget index for stopped
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : hasNextTarget ) {
        pathIndex[ index ]++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // setup next subtarget for those that have them
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : hasNextTarget ) {
        index_t localPathIndex = pathIndex[ index ];
        grid::Coord nextSubtarget = path[ index ]->points[ localPathIndex ];

        int rx = rand() % 5 - 2;
        int ry = rand() % 5 - 2;

        grid::Coord randomizedSubtarget = nextSubtarget;
        randomizedSubtarget.x += rx;
        randomizedSubtarget.y += ry;

        if ( validCollisionCell( state, randomizedSubtarget ) ) {
            subtarget[ index ].x = randomizedSubtarget.x;
            subtarget[ index ].y = randomizedSubtarget.y;
        } else {
            subtarget[ index ].x = nextSubtarget.x;
            subtarget[ index ].y = nextSubtarget.y;
        }
    }

    std::vector< index_t > canRepath;

    ////////////////////////////////////////////////////////////////////////////
    // select humans that can repath
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : running ) {
        canRepath.push_back( index );
    }
    for ( index_t index : noNextTarget ) {
        canRepath.push_back( index );
    }

    ////////////////////////////////////////////////////////////////////////////
    // tick repath timer
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : canRepath ) {
        repathTimer[ index ] -= deltaTime;
    }

    std::vector< index_t > needRepath;

    ////////////////////////////////////////////////////////////////////////////
    // select humans that need repathing
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : canRepath ) {
        if ( repathTimer[ index ] <= 0.0f ) {
            needRepath.push_back( index );
        }
    }
    for ( index_t index : noPath ) {
        needRepath.push_back( index );
    }

    std::vector< index_t > stalePaths;

    ////////////////////////////////////////////////////////////////////////////
    // select humans with stale paths
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stoppedAtTarget ) {
        if ( path[ index ] ) {
            stalePaths.push_back( index );
        }
    }
    for ( index_t index : needRepath ) {
        if ( path[ index ] ) {
            stalePaths.push_back( index );
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // delete finished paths
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stalePaths ) {
        delete path[ index ];
        path[ index ] = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////
    // repath humans to their targets
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : needRepath ) {
        path[ index ] =
            computePathForHuman( state, pos[ index ], target[ index ] );
        pathIndex[ index ] = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // randomize repath timer
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : needRepath ) {
        repathTimer[ index ] = ( std::rand() % 100 ) * 0.01 + 2.0f;
    }

    ////////////////////////////////////////////////////////////////////////////
    // select humans stopped at target
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index : stoppedAtTarget ) {
        stoppedAtTargetId.push_back( id[ index ] );
    }
}

////////////////////////////////////////////////////////////////////////////////

static void tickHumanAi( state::GameState & state ) {
    //
}

static void tickMassPathables( state::GameState & state ) {
    using index_t = int;

    state::TycoonSim & sim = state.tycoon.tycoonSim;

    const float step = 20.0f * state.tickTimestep;
    const float deltaTime = state.tickTimestep;

    std::vector< glm::vec2 > & pos = sim.customers.position;
    std::vector< glm::vec2 > & vel = sim.customers.velocity;
    std::vector< glm::vec2 > & subtarget = sim.customers.subtarget;
    std::vector< glm::vec2 > & target = sim.customers.target;
    std::vector< astar::Path * > & path = sim.customers.path;
    std::vector< index_t > & pathIndex = sim.customers.pathIndex;
    std::vector< float > & repathTimer = sim.customers.repathTimer;
    std::vector< state::id_t > & stoppedAtTargetId = sim.customersAtTarget;
    std::vector< state::id_t > & id = sim.customers.id;

    std::vector< glm::vec2 > dir;
    std::vector< float > len;

    ////////////////////////////////////////////////////////////////////////////
    // compute dir and len to target
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        glm::vec2 localDir = target[ index ] - pos[ index ];
        dir.push_back( localDir );
        len.push_back( glm::length( localDir ) );
    }

    ////////////////////////////////////////////////////////////////////////////
    // select humans at target
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        if ( len[ index ] < 2.0f ) {
            stoppedAtTargetId.push_back( id[ index ] );
        }
    }

    std::vector< pathGrid::Vector > pathVector;

    ////////////////////////////////////////////////////////////////////////////
    // get field direction at spots
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        grid::Coord coord;
        coord.x = pos[ index ].x;
        coord.y = pos[ index ].y;

        grid::Info & gridInfo = state.tycoon.collisionGridInfo;
        if ( grid::contains( gridInfo, coord ) ) {
            size_t gridIndex = grid::index( gridInfo, coord );
            pathVector.push_back( state.tycoon.pathGridField[ gridIndex ] );
        } else {
            pathVector.push_back( 0 );
        }
    }

    std::vector< glm::vec2 > pathVel;

    ////////////////////////////////////////////////////////////////////////////
    // turn field direction to vec2
    ////////////////////////////////////////////////////////////////////////////
    for ( pathGrid::Vector v : pathVector ) {
        glm::vec2 out = glm::vec2{ 1, 0 };
        switch ( v ) {
        case 0b1000:
            out = glm::vec2{ 1, 0 };
            break;
        case 0b0100:
            out = glm::vec2{ -1, 0 };
            break;
        case 0b0010:
            out = glm::vec2{ 0, 1 };
            break;
        case 0b0001:
            out = glm::vec2{ 0, -1 };
            break;

        case 0b1010:
            out = glm::vec2{ 0.7f, 0.7f };
            break;
        case 0b1001:
            out = glm::vec2{ 0.7f, -0.7f };
            break;
        case 0b0110:
            out = glm::vec2{ -0.7f, 0.7f };
            break;
        case 0b0101:
            out = glm::vec2{ -0.7f, -0.7f };
            break;

        case 0b1100:
            out = glm::vec2{ 1, 0 };
            break;
        case 0b0011:
            out = glm::vec2{ 0, 1 };
            break;
        case 0b1110:
            out = glm::vec2{ 1, 0 };
            break;
        case 0b0111:
            out = glm::vec2{ 0, 1 };
            break;

        case 0b1101:
            out = glm::vec2{ 1, 0 };
            break;
        case 0b1011:
            out = glm::vec2{ 0, 1 };
            break;
        case 0b1111:
            out = glm::vec2{ 1, 0 };
            break;
        default:
            // ERROR_LOG() << "WTF: " << (int) v << std::endl;
            break;
        }

        float randAngle = ( rand() % 4 ) * 0.25f * 6.28f;
        glm::vec2 randVec{ cos( randAngle ), sin( randAngle ) };

        out += randVec * 0.5f;

        pathVel.push_back( out );
    }

    ////////////////////////////////////////////////////////////////////////////
    // move humans forward
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        vel[ index ] += pathVel[ index ] * 1000.0f * deltaTime;
        float len = glm::length( vel[ index ] );

        if ( len > 100.0f ) {
            vel[ index ] *= 100.0f / len;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // move humans forward
    ////////////////////////////////////////////////////////////////////////////
    for ( index_t index = 0; index < pos.size(); index++ ) {
        float len = glm::length( vel[ index ] );
        if ( len > 0 ) {
            glm::vec2 newPos = pos[ index ] + vel[ index ] * step / len;

            grid::Coord coord;
            coord.x = newPos.x;
            coord.y = newPos.y;

            grid::Info & gridInfo = state.tycoon.collisionGridInfo;
            if ( grid::contains( gridInfo, coord ) ) {
                size_t gridIndex = grid::index( gridInfo, coord );

                if ( !state.tycoon.collisionGridData[ gridIndex ] ) {
                    pos[ index ] = newPos;
                }
            }
        }
    }
}

static void tickHumans( state::GameState & state ) {
    state::TycoonSim & sim = state.tycoon.tycoonSim;

    sim.customersAtTarget.clear();

    tickMassPathables( state );

    tickHumanAi( state );

    for ( state::id_t id : sim.customersAtTarget ) {
        removeHuman( state, id );
    }
}

////////////////////////////////////////////////////////////////////////////////

static void generateWallRect( state::GameState & state, int x, int y, int width,
                              int height ) {
    const int wallWidth = 5;
    const int wallLength = 15;

    const int top = x;
    const int left = y;

    // draw top
    for ( int i = 0; i < width; i++ ) {
        Rect rect;
        rect.x = left + wallLength * i;
        rect.y = top;
        rect.w = wallLength;
        rect.h = wallWidth;
        state.tycoon.walls.push_back( rect );
    }

    // draw bottom
    for ( int i = 0; i < width; i++ ) {
        Rect rect;
        rect.x = left + wallLength * i;
        rect.y = top + wallLength * height + wallWidth;
        rect.w = wallLength;
        rect.h = wallWidth;
        state.tycoon.walls.push_back( rect );
    }

    // draw left
    for ( int i = 0; i < height; i++ ) {
        Rect rect;
        rect.x = left;
        rect.y = top + wallLength * i + wallWidth;
        rect.w = wallWidth;
        rect.h = wallLength;
        state.tycoon.walls.push_back( rect );
    }

    // draw right
    for ( int i = 0; i < height; i++ ) {
        Rect rect;
        rect.x = left + wallLength * width - wallWidth;
        rect.y = top + wallLength * i + wallWidth;
        rect.w = wallWidth;
        rect.h = wallLength;
        state.tycoon.walls.push_back( rect );
    }
}

////////////////////////////////////////////////////////////////////////////////

static void generateWalls( state::GameState & state ) {
    generateWallRect( state, 20, 20, 15, 10 );
}

////////////////////////////////////////////////////////////////////////////////

static Rect handRect( state::GameState & state, const glm::vec2 & pos ) {
    Rect rect;

    glm::vec2 tableHSize = state.tycoon.textures.tableHSize;

    const int wallWidth = 5;
    const int wallLength = 15;

    switch ( state.tycoon.selectedTool ) {
    case state::TOOL_HUMAN:
    case state::TOOL_ERASE:
    case state::TOOL_SETWAYPOINT:
    case state::TOOL_TABLE:
        rect = { (int) state.tycoon.mousePosition.x - (int) tableHSize.x,
                 (int) state.tycoon.mousePosition.y - (int) tableHSize.y, 16,
                 16 };
        break;
    case state::TOOL_WALL:
        if ( state.tycoon.toolRotation % 2 ) {
            rect = { (int) state.tycoon.mousePosition.x - (int) wallWidth / 2,
                     (int) state.tycoon.mousePosition.y - (int) wallLength / 2,
                     wallWidth, wallLength };
        } else {
            rect = { (int) state.tycoon.mousePosition.x - (int) wallLength / 2,
                     (int) state.tycoon.mousePosition.y - (int) wallWidth / 2,
                     wallLength, wallWidth };
        }
        break;
    }

    return rect;
}

////////////////////////////////////////////////////////////////////////////////

static void computePath( state::GameState & state ) {
    // build collision grid
    grid::Info gridInfo;
    gridInfo.width = state.rendering.subRenderWidth;
    gridInfo.height = state.rendering.subRenderHeight;

    grid::Coord start;
    grid::Coord end;

    start.x = state.tycoon.pathEndpoints[ 0 ].x;
    start.y = state.tycoon.pathEndpoints[ 0 ].y;
    end.x = state.tycoon.pathEndpoints[ 1 ].x;
    end.y = state.tycoon.pathEndpoints[ 1 ].y;

    if ( !grid::contains( gridInfo, start.x, start.y ) ) {
        return;
    }

    if ( !grid::contains( gridInfo, end.x, end.y ) ) {
        return;
    }

    size_t gridSize = gridInfo.width * gridInfo.height;
    std::vector< int > grid( gridSize, 0 );

    for ( Rect wall : state.tycoon.walls ) {
        wall = math::marginRect( wall, -1 );

        for ( int i = 0; i < wall.w; i++ ) {
            for ( int j = 0; j < wall.h; j++ ) {
                int x = wall.x + i;
                int y = wall.y + j;

                if ( grid::contains( gridInfo, x, y ) ) {
                    auto index = grid::index( gridInfo, x, y );
                    grid[ index ] = 1;
                }
            }
        }
    }

    auto path = astar::shortestPath( gridInfo, grid, start, end, 1000 );

    // auto vec2Path = toVec2Vector( path.points );

    // auto vec2Splines = math::spline( vec2Path, 2.0f );

    state.tycoon.waypoints = path.points;
    state.tycoon.debugPoints = path.debugOpenPoints;
}

static void handleClick( state::GameState & state, glm::vec2 pos ) {

    // check for gui click

    for ( int i = 0; i < std::ssize( state.tycoon.selectorButtons ); i++ ) {
        if ( math::contains( state.tycoon.selectorButtons[ i ],
                             state.tycoon.mousePosition.x,
                             state.tycoon.mousePosition.y ) ) {

            state.tycoon.selectedTool = state::Tool( i );

            if ( i < std::ssize( kToolNames ) ) {
                addConsoleLine( state, "settool " + kToolNames[ i ] );
            } else {
                addConsoleLine( state, "settool " + std::to_string( i + 1 ) );
            }

            return;
        }
    }

    if ( state.tycoon.selectedTool == state::TOOL_TABLE ) {
        Rect tableRect = handRect( state, state.tycoon.mousePosition );

        glm::vec2 tablePos{ tableRect.x, tableRect.y };

        if ( !collidesWithWalls( state, tableRect ) ) {
            state.tycoon.tablePositions.push_back( tablePos );
            state.tycoon.tablePositionsAnim.push_back( tablePos );
        }

        state.tycoon.money -= 10;
    } else if ( state.tycoon.selectedTool == state::TOOL_WALL ) {
        Rect wallRect = handRect( state, state.tycoon.mousePosition );

        if ( !collidesWithWalls( state, wallRect ) ) {
            state.tycoon.walls.push_back( wallRect );
        }

        state.tycoon.money -= 5;

        computeCollisionGrid( state );
        computePath( state );
    } else if ( state.tycoon.selectedTool == state::TOOL_HUMAN ) {

        glm::vec2 center = state.tycoon.mousePosition;

        for ( int i = 0; i <= 50; i++ ) {

            for ( int j = 0; j <= 50; j++ ) {
                int dx = i - 25;
                int dy = j - 25;

                if ( dx * dx + dy * dy <= 25 * 25 ) {
                    glm::vec2 pos = center;
                    pos.x += dx;
                    pos.y += dy;
                    addHuman( state, pos );
                }
            }
        }

    } else if ( state.tycoon.selectedTool == state::TOOL_ERASE ) {
        for ( int i = 0; i < std::ssize( state.tycoon.walls ); i++ ) {
            if ( math::contains( state.tycoon.walls[ i ],
                                 state.tycoon.mousePosition.x,
                                 state.tycoon.mousePosition.y ) ) {
                // remove wall
                state.tycoon.walls.erase( state.tycoon.walls.begin() + i );
            }
        }
        computeCollisionGrid( state );
        computePath( state );
    } else if ( state.tycoon.selectedTool == 4 ) {
        // int n = state.tycoon.waypointNum;

        state.tycoon.pathEndpoints[ 0 ] = pos;

        // state.tycoon.waypointNum = ( n + 1 ) % 2;

        computePath( state );
    } else if ( state.tycoon.selectedTool == 5 ) {
        // int n = state.tycoon.waypointNum;

        state.tycoon.pathEndpoints[ 1 ] = pos;

        // state.tycoon.waypointNum = ( n + 1 ) % 2;

        computePath( state );
    }
}

////////////////////////////////////////////////////////////////////////////////

static void initAnimations( state::GameState & state ) {
    state.tycoon.tablePositionsAnim.clear();

    for ( auto & pos : state.tycoon.tablePositions ) {
        state.tycoon.tablePositionsAnim.push_back( pos );
    }
}

////////////////////////////////////////////////////////////////////////////////

static void animate( state::GameState & state ) {
    for ( int i = 0; i < std::ssize( state.tycoon.tablePositions ); i++ ) {
        glm::vec2 & pos = state.tycoon.tablePositionsAnim[ i ];
        glm::vec2 & target = state.tycoon.tablePositions[ i ];

        glm::vec2 dir = target - pos;
        float len = glm::length( dir );

        if ( len > 10.0 ) {
            pos += dir * ( 400.0f / len ) * state.renderTimestep;
        } else {
            pos = target;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

static void resize( state::GameState & state, int width, int height ) {

    state::Rendering & rendering = state.rendering;
    state::Tycoon & tycoon = state.tycoon;

    rendering.renderWidth = width;
    rendering.renderHeight = height;
    rendering.subRenderWidth = width / rendering.pixelelizeFactor;
    rendering.subRenderHeight = height / rendering.pixelelizeFactor;
    tycoon.consoleRenderWidth =
        rendering.subRenderWidth * rendering.pixelelizeFactor;
    tycoon.consoleRenderHeight =
        rendering.subRenderHeight * rendering.pixelelizeFactor;

    state.tycoon.projMatrix =
        glm::ortho( 0.0f, (float) rendering.subRenderWidth,
                    (float) rendering.subRenderHeight, 0.0f );

    //{
    //    engine::genTexture( tycoon.lastFrame, rendering.subRenderWidth,
    //                        rendering.subRenderHeight );
    //}

    {
        engine::genTexture( tycoon.frambufferTexture1, rendering.subRenderWidth,
                            rendering.subRenderHeight );
        // set texture on framebuffer
        glBindFramebuffer( GL_FRAMEBUFFER, tycoon.framebuffer1 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, tycoon.frambufferTexture1, 0 );
    }

    {
        engine::genTexture( tycoon.frambufferTexture2,
                            tycoon.consoleRenderWidth,
                            tycoon.consoleRenderHeight );
        // set texture on framebuffer
        glBindFramebuffer( GL_FRAMEBUFFER, tycoon.framebuffer2 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, tycoon.frambufferTexture2, 0 );
    }

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    computeCollisionGrid( state );
}

////////////////////////////////////////////////////////////////////////////////

static void initShaders( state::GameState & state ) {
    state::Tycoon & tycoon = state.tycoon;
    state::DefaultShader2D & shader = tycoon.defaultShader;

    shader.program.reset( glCreateProgram() );
    gfx::ShaderBuilder()
        .attach( res::directory() / "shaders/simple2d.vert", GL_VERTEX_SHADER )
        .attach( res::directory() / "shaders/simple2d.frag",
                 GL_FRAGMENT_SHADER )
        .build( shader.program );

    tycoon.blurShader.program.reset( glCreateProgram() );
    gfx::ShaderBuilder()
        .attach( res::directory() / "shaders/blur.vert", GL_VERTEX_SHADER )
        .attach( res::directory() / "shaders/blur.frag", GL_FRAGMENT_SHADER )
        .build( tycoon.blurShader.program );

    tycoon.consoleShader.program.reset( glCreateProgram() );
    gfx::ShaderBuilder()
        .attach( res::directory() / "shaders/console.vert", GL_VERTEX_SHADER )
        .attach( res::directory() / "shaders/console.frag", GL_FRAGMENT_SHADER )
        .build( tycoon.consoleShader.program );

    shader.uMat = gfx::findUniform( shader.program, "uMat" );
    shader.uTexture = gfx::findUniform( shader.program, "uTexture" );
    shader.uUseTexture = gfx::findUniform( shader.program, "uUseTexture" );
    shader.uTint = gfx::findUniform( shader.program, "uTint" );

    tycoon.blurShader.uTexture =
        gfx::findUniform( tycoon.blurShader.program, "uTexture" );
    tycoon.blurShader.uBlurStep =
        gfx::findUniform( tycoon.blurShader.program, "uBlurStep" );

    tycoon.consoleShader.uTexture =
        gfx::findUniform( tycoon.consoleShader.program, "uTexture" );
    tycoon.consoleShader.uGlowFactor =
        gfx::findUniform( tycoon.consoleShader.program, "uGlowFactor" );
    tycoon.consoleShader.uWipeHeight =
        gfx::findUniform( tycoon.consoleShader.program, "uWipeHeight" );
}

////////////////////////////////////////////////////////////////////////////////

void init( state::GameState & state ) {
    std::srand( 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // init
    state::Tycoon & tycoon = state.tycoon;

    state.tycoon.consoleFactor = 1;
    state.rendering.pixelelizeFactor = 4 * state.tycoon.consoleFactor;

    gfx::initBuffer( tycoon.unitQuad );
    gfx::writeBuffer( tycoon.unitQuad, kUnitQuadVertices, kUnitQuadUvs );

    gfx::initBuffer( tycoon.topLeftUnitQuad );
    gfx::writeBuffer( tycoon.topLeftUnitQuad, kTopLeftUnitQuadVertices,
                      kUnitQuadUvs );

    gfx::initBuffer( tycoon.arrowHead );
    gfx::writeBuffer( tycoon.arrowHead, kArrowHeadVertices, kArrowHeadUvs );

    initShaders( state );

    tycoon.pickupTexture.init();
    engine::loadTexture( tycoon.pickupTexture,
                         res::directory() / "textures/tycoon/table1.png" );

    tycoon.textures.miku.init();
    engine::loadTexture( tycoon.textures.miku,
                         res::directory() / "textures/tycoon/miku.png" );
    tycoon.textures.kitchen.init();
    engine::loadTexture( tycoon.textures.kitchen,
                         res::directory() / "textures/tycoon/kitchen1.png" );
    tycoon.textures.table.init();
    engine::loadTexture( tycoon.textures.table,
                         res::directory() / "textures/tycoon/table1.png" );

    tycoon.textures.tableHSize = glm::vec2{ 8, 8 };

    // init framebuffers
    tycoon.lastFrame.init();
    tycoon.framebuffer1.init();
    tycoon.frambufferTexture1.init();
    tycoon.framebuffer2.init();
    tycoon.frambufferTexture2.init();

    engine::loadFont( tycoon.font, res::directory() / "fonts/bit.fnt" );
    gfx::initTextBuffer( tycoon.textBuffer );
    gfx::initTextBuffer( tycoon.moneyTextBuffer );
    gfx::initTextBuffer( tycoon.consoleTextBuffer );

    gfx::writeTextBuffer( tycoon.textBuffer, tycoon.font, "Running.",
                          glm::vec2{ 0.0f, 0.0f }, 1.0f );

    // tycoon.tablePositions.push_back( glm::vec2( 10.0f, 10.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 8.0f, 8.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 9.0f, 3.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 2.0f, 7.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 6.0f, 1.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 1.0f, 6.0f ) );
    // tycoon.tablePositions.push_back( glm::vec2( 1.0f, 1.0f ) );

    generateWalls( state );

    initAnimations( state );

    tycoon.elapsed = 0.0f;
    tycoon.moveTimer = 2.0f;
    tycoon.lastIndex = 0;

    tycoon.runningAnimationTimer = 0.0f;
    tycoon.runningAnimationIndex = 0;

    tycoon.money = 1500;
    tycoon.moneyDisplayed = 1500;

    tycoon.selectedTool = state::Tool( 0 );
    tycoon.toolRotation = 0;
    tycoon.console.timeLeft = 0;

    initModule( state );
}

////////////////////////////////////////////////////////////////////////////////

void input( state::GameState & state, InputEvent e, InputEventData data ) {
    // input
    switch ( e ) {
    case InputEvent::Click:
        handleClick( state,
                     screenToView( state, data.pos[ 0 ], data.pos[ 1 ] ) );
        break;
    case InputEvent::RotateTool:
        state.tycoon.toolRotation = ( state.tycoon.toolRotation + 1 ) % 4;
        break;
    case InputEvent::MoveUp:
    case InputEvent::MoveDown:
    case InputEvent::MoveLeft:
    case InputEvent::MoveRight:
        break;
    case InputEvent::Resize:
        resize( state, data.size[ 0 ], data.size[ 1 ] );
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void tick( state::GameState & state ) {
    tryReloadModule( state );

    state.tycoon.mousePosition = screenToView(
        state, state.shared.mouseCursor[ 0 ], state.shared.mouseCursor[ 1 ] );

    // tick
    state::Tycoon & tycoon = state.tycoon;

    tycoon.moveTimer -= state.tickTimestep;
    tycoon.runningAnimationTimer -= state.tickTimestep;

    if ( tycoon.console.timeLeft > 0.0f )
        tycoon.console.timeLeft -= state.tickTimestep;

    if ( tycoon.moneyDisplayed < tycoon.money ) {
        tycoon.moneyDisplayed++;
    } else if ( tycoon.moneyDisplayed > tycoon.money ) {
        tycoon.moneyDisplayed--;
    }

    // if ( tycoon.moveTimer < 0.0f ) {
    //     tycoon.moveTimer = 0.5f * ( rand() % 5 );

    //    int i = rand() % tycoon.tablePositions.size();

    //    tycoon.tablePositions[ i ].x =
    //        ( rand() % state.rendering.subRenderWidth );
    //    tycoon.tablePositions[ i ].y =
    //        ( rand() % state.rendering.subRenderHeight );
    //    tycoon.lastIndex = i;

    //    // std::cout << "\a"; // beep lmao
    //    // std::cout.flush();
    //}

    if ( tycoon.runningAnimationTimer < 0.0f ) {
        tycoon.runningAnimationTimer = 0.1f;
        tycoon.runningAnimationIndex = ( tycoon.runningAnimationIndex + 1 ) % 4;

        std::string text;
        switch ( tycoon.runningAnimationIndex ) {
        case 0:
            text = "RUNNING / ";
            break;
        case 1:
            text = "RUNNING - ";
            break;
        case 2:
            text = "RUNNING \\ ";
            break;
        case 3:
            text = "RUNNING | ";
            break;
        }

        state.inputLag = 0;
        int lag = (int) ( state.inputLag * 1000 );
        int fps = (int) std::ceil( 1.0f / state.renderTimestep );
        text += " FPS = " + std::to_string( fps );

        gfx::writeTextBuffer( tycoon.textBuffer, tycoon.font, text,
                              glm::vec2{ 0.0f, 0.0f }, 1.0f );
    }

    std::string moneyString =
        "BAL = $" + std::to_string( tycoon.moneyDisplayed );
    gfx::writeTextBuffer( tycoon.moneyTextBuffer, tycoon.font, moneyString,
                          glm::vec2{ 0.0f, 0.0f }, 1.0f );

    tickHumans( state );

    iteratePathGrids( state );
}

////////////////////////////////////////////////////////////////////////////////

static void renderSelector( state::GameState & state ) {
    glm::mat4 & proj = state.tycoon.projMatrix;
    state::DefaultShader2D & shader = state.tycoon.defaultShader;

    int screenWidth = state.rendering.subRenderWidth;
    int screenHeight = state.rendering.subRenderHeight;

    // reset everything to white
    gfx::setUniform( shader.uTint, glm::vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
    gfx::setUniform( shader.uUseTexture, 0 );

    Rect barRect;
    {
        int barHeight = screenHeight * 0.15f;
        barRect.x = 0;
        barRect.y = screenHeight - barHeight;
        barRect.w = screenWidth;
        barRect.h = barHeight;

        barRect = math::marginRect( barRect, 10 );

        glm::mat4 t = spriteTransform( barRect );

        gfx::setUniform( shader.uMat, proj * t );
        gfx::draw( state.tycoon.topLeftUnitQuad );
    }

    int buttonWidth = barRect.h;

    state.tycoon.selectorButtons.clear();

    for ( int i = 0; i < 10; i++ ) {
        Rect buttonRect;
        buttonRect.x = barRect.x + buttonWidth * i;
        buttonRect.y = barRect.y;
        buttonRect.w = buttonWidth;
        buttonRect.h = barRect.h;

        if ( i == state.tycoon.selectedTool ) {
            buttonRect = math::marginRect( buttonRect, 3 );
        } else {
            buttonRect = math::marginRect( buttonRect, 5 );
        }

        if ( math::contains( buttonRect, state.tycoon.mousePosition.x,
                             state.tycoon.mousePosition.y ) ) {
            gfx::setUniform( shader.uTint,
                             glm::vec4( 0.3f, 0.3f, 0.8f, 1.0f ) );
        } else {
            if ( i == state.tycoon.selectedTool ) {
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 0.3f, 0.3f, 0.3f, 1.0f ) );
            } else {
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ) );
            }
        }

        glm::mat4 t = spriteTransform( buttonRect );

        gfx::setUniform( shader.uMat, proj * t );
        gfx::draw( state.tycoon.topLeftUnitQuad );

        state.tycoon.selectorButtons.push_back( buttonRect );
    }
}

////////////////////////////////////////////////////////////////////////////////

void renderHand( state::GameState & state ) {
    glm::mat4 & proj = state.tycoon.projMatrix;
    state::DefaultShader2D & shader = state.tycoon.defaultShader;

    Rect rect = handRect( state, state.tycoon.mousePosition );
    Rect renderRect;

    switch ( state.tycoon.selectedTool ) {
    case state::TOOL_SETWAYPOINT:
    case state::TOOL_HUMAN:
    case state::TOOL_ERASE:
        return;
    case state::TOOL_TABLE:
        glBindTexture( GL_TEXTURE_2D, state.tycoon.textures.table );
        renderRect = rect;
        break;
    case state::TOOL_WALL:
        gfx::setUniform( shader.uUseTexture, 0 );
        renderRect = math::marginRect( rect, 1 );
        {
            gfx::setUniform( shader.uTint,
                             glm::vec4( 0.0f, 0.5f, 1.0f, 1.0f ) );
            glm::mat4 objTransform = spriteTransform( rect );
            gfx::setUniform( shader.uMat, proj * objTransform );
            gfx::draw( state.tycoon.topLeftUnitQuad );
        }
        gfx::setUniform( shader.uTint, glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
        break;
    }

    {
        glm::mat4 objTransform = spriteTransform( renderRect );

        if ( collidesWithWalls( state, rect ) ) {
            gfx::setUniform( shader.uTint,
                             glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );
        }

        gfx::setUniform( shader.uMat, proj * objTransform );
        gfx::draw( state.tycoon.topLeftUnitQuad );
    }
}

////////////////////////////////////////////////////////////////////////////////

void render( state::GameState & state ) {
    // render

    state.tycoon.elapsed += state.renderTimestep;

    animate( state );

    int screenWidth = state.rendering.subRenderWidth;
    int screenHeight = state.rendering.subRenderHeight;
    glm::vec2 screenSize{ screenWidth, screenHeight };

    // use framebuffer 1
    {
        glBindFramebuffer( GL_FRAMEBUFFER, state.tycoon.framebuffer1 );
        glViewport( 0, 0, state.rendering.subRenderWidth,
                    state.rendering.subRenderHeight );
        glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    }

    // do normal pass
    {
        glm::mat4 & proj = state.tycoon.projMatrix;

        state::DefaultShader2D & shader = state.tycoon.defaultShader;

        glUseProgram( shader.program );

        gfx::setUniform( shader.uUseTexture, 0 );

        // draw background
        //{
        //    gfx::setUniform( shader.uTint,
        //                     glm::vec4( 0.2f, 0.2f, 0.2f, 1.0f ) );

        //    glm::mat4 objTransform( 1.0f );
        //    objTransform = glm::translate(
        //        objTransform,
        //        glm::vec3( state.rendering.subRenderWidth * 0.5f,
        //                   state.rendering.subRenderHeight * 0.5f, 0.0f ) );
        //    objTransform = glm::scale(
        //        objTransform,
        //        glm::vec3( state.rendering.subRenderWidth,
        //                   state.rendering.subRenderHeight, 1.0f ) );

        //    gfx::setUniform( shader.uMat, proj * objTransform );
        //    gfx::draw( state.tycoon.unitQuad );
        //}

        // draw walls

        gfx::setUniform( shader.uUseTexture, 0 );
        gfx::setUniform( shader.uTexture, 0 );

        // draw cost map
        {

            std::vector< int > & costs = state.tycoon.pathGridCosts;
            grid::Info & gridInfo = state.tycoon.collisionGridInfo;
            for ( size_t i = 0; i < grid::size( gridInfo ); i++ ) {
                int cost = costs[ i ];
                grid::Coord point = grid::coord( gridInfo, i );

                gfx::setUniform(
                    shader.uTint,
                    glm::vec4( std::max( 0.0f, 1.0f - cost * 0.013f ), 0.0f,
                               0.0f, 1.0f ) );

                Rect rect;
                rect.x = point.x;
                rect.y = point.y;
                rect.w = 1;
                rect.h = 1;

                glm::mat4 objTransform = spriteTransform( rect );
                gfx::setUniform( shader.uMat, proj * objTransform );
                gfx::draw( state.tycoon.topLeftUnitQuad );
            }
        }

        {
            for ( auto & wall : state.tycoon.walls ) {
                Rect rect;

                bool editingMode =
                    state.tycoon.selectedTool == state::TOOL_WALL ||
                    state.tycoon.selectedTool == state::TOOL_ERASE;

                if ( editingMode ) {
                    rect = math::marginRect( wall, 1 );
                } else {
                    rect = math::marginRect( wall, 0 );
                }

                if ( editingMode ) {
                    gfx::setUniform( shader.uTint,
                                     glm::vec4( 0.0f, 0.5f, 1.0f, 1.0f ) );
                    glm::mat4 objTransform = spriteTransform( wall );
                    gfx::setUniform( shader.uMat, proj * objTransform );
                    gfx::draw( state.tycoon.topLeftUnitQuad );
                }

                if ( state.tycoon.selectedTool == state::TOOL_ERASE &&
                     math::contains( wall, state.tycoon.mousePosition.x,
                                     state.tycoon.mousePosition.y ) ) {
                    gfx::setUniform( shader.uTint,
                                     glm::vec4( 1.0f, 0.1f, 0.1f, 1.0f ) );
                } else {
                    gfx::setUniform( shader.uTint,
                                     glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
                }

                glm::mat4 objTransform = spriteTransform( rect );
                gfx::setUniform( shader.uMat, proj * objTransform );
                gfx::draw( state.tycoon.topLeftUnitQuad );
            }
        }

        for ( auto & human : state.tycoon.tycoonSim.customers.position ) {
            Rect rect;
            rect.x = human.x;
            rect.y = human.y;
            rect.w = 1;
            rect.h = 1;

            gfx::setUniform( shader.uTint,
                             glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f ) );

            glm::mat4 objTransform = spriteTransform( rect );
            gfx::setUniform( shader.uMat, proj * objTransform );
            gfx::draw( state.tycoon.topLeftUnitQuad );
        }

        gfx::setUniform( shader.uUseTexture, 1 );
        glBindTexture( GL_TEXTURE_2D, state.tycoon.textures.table );
        gfx::setUniform( shader.uTexture, 0 );

        for ( int i = 0; i < (int) state.tycoon.tablePositions.size(); i++ ) {

            glm::vec2 pos = state.tycoon.tablePositionsAnim[ i ];

            if ( i == state.tycoon.lastIndex &&
                 state.tycoon.moveTimer > 1.0f ) {
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
            } else {
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
            }

            glm::mat4 objTransform = spriteTransform( pos.x, pos.y, 16, 16 );
            gfx::setUniform( shader.uMat, proj * objTransform );
            gfx::draw( state.tycoon.topLeftUnitQuad );
        }

        // reset everything to white
        gfx::setUniform( shader.uTint, glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

        {
            glBindTexture( GL_TEXTURE_2D, state.tycoon.textures.miku );
            glm::mat4 objTransform =
                spriteTransform( screenWidth - 50, 0, 50, 50 );
            gfx::setUniform( shader.uMat, proj * objTransform );
            gfx::draw( state.tycoon.topLeftUnitQuad );
        }

        // render gui in screen coordinate frame
        renderHand( state );
        renderSelector( state );

        // reset everything to white
        gfx::setUniform( shader.uTint, glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

        {
            gfx::setUniform( shader.uUseTexture, 1 );
            glBindTexture( GL_TEXTURE_2D, state.tycoon.font.texture );

            int x = screenWidth - (int) state.tycoon.textBuffer.size.width;
            int y = screenHeight - (int) state.tycoon.textBuffer.size.height;

            glm::mat4 t = spriteTransform( x, y, 1, 1 );

            gfx::setUniform( shader.uMat, proj * t );
            gfx::draw( state.tycoon.textBuffer );

            // x = screenWidth - (int) state.tycoon.moneyTextBuffer.size.width;
            // t = spriteTransform( x / 2, 0, 1, 1 );
            // gfx::setUniform( shader.uMat, proj * t );
            // gfx::draw( state.tycoon.moneyTextBuffer );

            if ( state.tycoon.console.timeLeft > 0.0f ) {
                gfx::setUniform( shader.uUseTexture, 0 );
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 0.0f, 0.0f, 0.0f, 1.0f ) );

                Rect consoleRect;
                consoleRect.x = 0;
                consoleRect.y = 0;
                consoleRect.w = state.tycoon.consoleTextBuffer.size.width;
                consoleRect.h = state.tycoon.consoleTextBuffer.size.height;
                t = spriteTransform( consoleRect );
                gfx::setUniform( shader.uMat, proj * t );
                gfx::draw( state.tycoon.topLeftUnitQuad );

                gfx::setUniform( shader.uUseTexture, 1 );
                gfx::setUniform( shader.uTint,
                                 glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

                t = spriteTransform( 0, 0, 1, 1 );
                gfx::setUniform( shader.uMat, proj * t );
                gfx::draw( state.tycoon.consoleTextBuffer );
            }
        }

        // render waypoints
        {
            gfx::setUniform( shader.uUseTexture, 0 );
            gfx::setUniform( shader.uTint,
                             glm::vec4( 1.0f, 1.0f, 0.0f, 1.0f ) );
            for ( int i = 1; i < std::ssize( state.tycoon.waypoints ); i++ ) {
                grid::Coord start = state.tycoon.waypoints[ i - 1 ];
                grid::Coord end = state.tycoon.waypoints[ i ];

                glm::mat4 t = lineTransform( start.x, start.y, end.x, end.y );
                gfx::setUniform( shader.uMat, proj * t );
                gfx::draw( state.tycoon.topLeftUnitQuad );
            }

            gfx::setUniform( shader.uTint,
                             glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f ) );

            for ( int i = 0; i < std::ssize( state.tycoon.waypoints ); i++ ) {
                grid::Coord point = state.tycoon.waypoints[ i ];

                Rect rect;
                rect.x = point.x - 1;
                rect.y = point.y - 1;
                rect.w = 3;
                rect.h = 3;

                glm::mat4 objTransform = spriteTransform( rect );
                gfx::setUniform( shader.uMat, proj * objTransform );
                gfx::draw( state.tycoon.topLeftUnitQuad );
            }

            // gfx::setUniform( shader.uTint,
            //                  glm::vec4( 1.0f, 0.0f, 0.0f, 1.0f ) );
            // for ( int i = 0; i < std::ssize( state.tycoon.debugPoints ); i++
            // ) {
            //     grid::Coord point = state.tycoon.debugPoints[ i ];

            //    Rect rect;
            //    rect.x = point.x;
            //    rect.y = point.y;
            //    rect.w = 1;
            //    rect.h = 1;

            //    glm::mat4 objTransform = spriteTransform( rect );
            //    gfx::setUniform( shader.uMat, proj * objTransform );
            //    gfx::draw( state.tycoon.topLeftUnitQuad );
            //}
        }
    }

    // use framebuffer 2
    {
        glBindFramebuffer( GL_FRAMEBUFFER, state.tycoon.framebuffer2 );
        glViewport( 0, 0, state.tycoon.consoleRenderWidth,
                    state.tycoon.consoleRenderHeight );
        glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    }

    // do console pass
    {
        state::ConsoleShader & shader = state.tycoon.consoleShader;

        glUseProgram( shader.program );
        gfx::setUniform( shader.uTexture, 0 );
        // gfx::setUniform( shader.uGlowFactor,
        //                  (float) sin( state.tycoon.elapsed * 5.0f ) *
        //                  0.05f +
        //                      0.95f );
        gfx::setUniform( shader.uWipeHeight,
                         std::fmod( state.tycoon.elapsed * 12.1f, 1.0f ) );
        gfx::setUniform( shader.uGlowFactor, 1.0f );

        //////////////////////////////////////////
        //////////////////////////////////////////
        gfx::setUniform( shader.uWipeHeight, 0.0f );
        //////////////////////////////////////////
        //////////////////////////////////////////

        glBindTexture( GL_TEXTURE_2D, state.tycoon.frambufferTexture1 );

        gfx::draw( state.tycoon.unitQuad );
    }

    // use default framebuffer
    {
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        glViewport( 0, 0, state.rendering.renderWidth,
                    state.rendering.renderHeight );
        glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    }

    // do blur pass

    {
        state::BlurShader & shader = state.tycoon.blurShader;

        glUseProgram( shader.program );
        gfx::setUniform( shader.uTexture, 0 );
        // gfx::setUniform( shader.uBlurStep,
        //                  glm::vec2( 1.0f /
        //                  state.rendering.subRenderWidth,
        //                             1.0f /
        //                             state.rendering.subRenderHeight )
        //                             );
        gfx::setUniform( shader.uBlurStep,
                         glm::vec2( 0.0f / state.rendering.renderWidth,
                                    0.0f / state.rendering.renderHeight ) );

        glBindTexture( GL_TEXTURE_2D, state.tycoon.frambufferTexture2 );

        gfx::draw( state.tycoon.unitQuad );
    }
}

static bool moduleInitialized = false;

static void initModule( state::GameState & state ) {
    moduleInitialized = true;
}

static void reloadModule( state::GameState & state ) {
    state.tycoon.money -= 10;

    // state.rendering.pixelelizeFactor = 4;
    // resize( state, state.rendering.renderWidth, state.rendering.renderHeight
    // ); initShaders( state );
}

static void tryReloadModule( state::GameState & state ) {
    if ( !moduleInitialized ) {
        reloadModule( state );
    }

    moduleInitialized = true;
}

} // namespace tycoon
