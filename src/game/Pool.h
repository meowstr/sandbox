#pragma once

#include <vector>

namespace data {

using Id = long unsigned int;

template < typename T > using List = std::vector< T >;

struct PoolNode {
    Id id;
    PoolNode * next;
    bool active;
};

struct Pool {
    std::vector< PoolNode > nodes;
    std::vector< Id > activeIds;

    PoolNode * freeHead = nullptr;

    template < typename T > void sync( std::vector< T > & v ) {
        v.resize( nodes.size() );
    }

    Id get() {
        if ( !freeHead ) {
            PoolNode node;
            node.id = nodes.size();
            node.next = nullptr; // not necessary
            node.active = true;
            nodes.push_back( node );
            return node.id;
        } else {
            Id id = freeHead->id;
            freeHead->active = true;
            freeHead->next = nullptr; // not necessary
            freeHead = freeHead->next;
            return id;
        }
    }

    void free( Id id ) {
        PoolNode * oldHead = freeHead;
        freeHead = &nodes.at( id );
        freeHead->active = false;
        freeHead->next = oldHead;
    }

    void flush() {
        activeIds.clear();
        for ( const PoolNode & node : nodes ) {
            if ( node.active ) {
                activeIds.push_back( node.id );
            }
        }
    }
};

} // namespace data
