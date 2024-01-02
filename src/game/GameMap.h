#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

namespace map {

// class MapKey {};

////////////////////////////////////////////////////////////////////////////////

class Map {

  public:
    Map( std::filesystem::path path );
    Map() {}

    std::optional< std::string > query( std::string key ) const;

    void put( std::string key, std::string value );

    void write( std::filesystem::path path );

    template < typename S >
    std::optional< typename S::Type > queryTyped( std::string key ) const {
        auto val = query( key );
        if ( val ) {
            S s;
            return s.read( val.value() );
        } else {
            return std::nullopt;
        }
    }

    template < typename S >
    void putTyped( std::string key, typename S::Type value ) {
        S s;
        put( key, s.write( value ) );
    }

    // static std::string makeKey( const std::vector< std::string > & path );

  private:
    std::map< std::string, std::string > mMap;
};

namespace serialize {

////////////////////////////////////////////////////////////////////////////////

struct String {
    using Type = std::string;

    Type read( const std::string & str ) {
        return str;
    }

    std::string write( Type x ) {
        return x;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct Bool {
    using Type = bool;

    Type read( const std::string & str ) {
        return str == "yes";
    }

    std::string write( Type x ) {
        return x ? "yes" : "no";
    }
};

////////////////////////////////////////////////////////////////////////////////

struct Int {
    using Type = int;

    Type read( const std::string & str ) {
        return std::atoi( str.c_str() );
    }

    std::string write( Type x ) {
        return std::to_string( x );
    }
};

////////////////////////////////////////////////////////////////////////////////

struct Float {
    using Type = float;

    Type read( const std::string & str ) {
        return (float) std::atof( str.c_str() );
    }

    std::string write( Type x ) {
        return std::to_string( x );
    }
};

////////////////////////////////////////////////////////////////////////////////

template < typename S > struct List {
    using Type = std::vector< typename S::Type >;

    Type read( const std::string & str ) {
        auto startBracket = str.find_first_of( '[' );
        auto endBracket = str.find_last_of( ']' );

        if ( startBracket == std::string::npos ||
             endBracket == std::string::npos ) {
            throw std::runtime_error( "Could not parse \"" + str +
                                      "\" as list: missing [] brackets" );
        }
        auto len = endBracket - startBracket;

        if ( len <= 0 ) {
            throw std::runtime_error(
                "Could not parse \"" + str +
                "\" as list: incorrect order of brackets" );
        }

        std::string innerStr = str.substr( startBracket + 1, len - 1 );

        std::stringstream ss( innerStr );

        Type v;
        S s;
        while ( ss ) {
            std::string token;
            ss >> token;

            // skip ending token
            if ( token.empty() )
                continue;

            // std::cout << "[" << token << "]" << std::endl;

            v.push_back( s.read( token ) );
        }

        return v;
    }

    std::string write( Type x ) {
        std::stringstream ss;

        ss << "[ ";

        S s;
        for ( auto e : x ) {
            ss << s.write( e );
            ss << " ";
        }

        ss << "]";

        return ss.str();
    }
};

} // namespace serialize

} // namespace map
