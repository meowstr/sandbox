#include "Astar.h"

#include <algorithm>

namespace astar {

void init( State & state, Node::Id start, int heuristic ) {
    state.nodes.clear();
    state.open.clear();

    Node root;

    root.id = start;
    root.parent = -1; // special
    root.h = heuristic;
    root.g = 0;
    root.f = heuristic;

    state.nodes.push_back( root );
    state.open.push_back( 0 );
    state.root = 0;
}

bool run( State & state ) {

    if ( !state.open.empty() ) {
        // sort open by f score
        std::sort( state.open.begin(), state.open.end(),
                   [ &state ]( Index a, Index b ) {
                       return state.nodes[ a ].f < state.nodes[ b ].f;
                   } );

        state.root = state.open[ 0 ];

        // remove from open
        state.open.erase( state.open.begin() );

        return true;
    } else {
        return false;
    }
}

Node::Id root( State & state ) {
    return state.nodes[ state.root ].id;
}

void addNeighbor( State & state, Node::Id id, int distance, int heuristic ) {
    // search for in current node list
    auto it = std::find_if( state.nodes.begin(), state.nodes.end(),
                            [ id ]( const Node & n ) { return n.id == id; } );

    Index itIndex = (Index) ( it - state.nodes.begin() );

    Node & rootNode = state.nodes[ state.root ];

    int h = heuristic;
    int g = rootNode.g + distance;
    int f = h + g;

    if ( it == state.nodes.end() ) {
        // add a new node
        Node node;
        node.id = id;
        node.h = h;
        node.g = g;
        node.f = f;
        node.parent = rootNode.id;

        state.open.push_back( (Index) state.nodes.size() );
        state.nodes.push_back( node );
    } else {
        // update current node
        Node & node = *it;
        if ( f < node.f ) {
            node.h = h;
            node.g = g;
            node.f = f;
            node.parent = rootNode.id;
        }

        // add to open list if not there already
        auto it2 = std::find( state.open.begin(), state.open.end(), itIndex );
        if ( it2 == state.open.end() ) {
            // add to open
            state.open.push_back( itIndex );
        }
    }
}

Path generatePath( State & state ) {
    Path path;

    // start at root and rewind until start (parent == -1)
    Index index = state.root;
    while ( index != -1 ) {
        Node & node = state.nodes[ index ];
        path.nodes.push_back( node.id );

        index = node.parent;
    }

    return path;
}

} // namespace astar
