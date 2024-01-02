#pragma once

#include <vector>

namespace astar {
using Index = long;

struct Node {
    using Id = long;

    Id id;
    Index parent;

    int h;
    int g;
    int f;
};

struct State {

    std::vector< Node > nodes; // TODO: upgrade to hash map?
    std::vector< Index > open; // TODO: upgrade to min heap

    Index root; // current node (end of path)
};

struct Path {
    std::vector< Node::Id > nodes;
};

void init( State & state, Node::Id start, int heuristic );

bool run( State & state );

Node::Id root( State & state );

void addNeighbor( State & state, Node::Id id, int distance, int heuristic );

Path generatePath( State & state );

} // namespace astar
