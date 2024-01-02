#include "CommandLineParser.h"

#include <stdexcept>

namespace commandLineParser {

CommandLineOptions CommandLineParser::parse( int argc, char ** argv ) {
    // if they do the single '-'
    // ./app -t vr
    // ./app --apptype vr
    // ./app --enablething

    std::string programName = argv[ 0 ];
    std::map< std::string, std::string > flagMap;

    // fill map with default values
    for ( auto && x : mFlagNameToDefault ) {
        flagMap[ x.second.name ] = x.second.value;
    }

    for ( int i = 1; i < argc; i++ ) {
        // parse a flag

        std::string token = argv[ i ];

        CommandFlag commandFlag;
        if ( token.starts_with( "--" ) ) {
            // long flag
            std::string flagName = token.substr( 2 );
            if ( !mFlagNameToDefault.contains( flagName ) ) {
                // not a flag
                throw std::runtime_error( "Invalid command line flag: " +
                                          flagName );
            }
            commandFlag = mFlagNameToDefault[ flagName ];
        } else if ( token.starts_with( "-" ) ) {
            // short flag
            std::string flagName = token.substr( 1 );
            if ( !mFlagShortNameToDefault.contains( flagName ) ) {
                // not a flag
                throw std::runtime_error( "Invalid command line flag: " +
                                          flagName );
            }
            commandFlag = mFlagShortNameToDefault[ flagName ];
        } else {
            // not a flag
            throw std::runtime_error( "Invalid command line argument: " +
                                      token );
        }

        if ( commandFlag.value == "false" ) {
            // this is a boolean flag
            flagMap[ commandFlag.name ] = "true";
        } else {
            // this is a normal flag
            i++;
            if ( i >= argc ) {
                throw std::runtime_error(
                    "Expected value for command line flag: " +
                    commandFlag.name );
            }
            std::string flagValue = argv[ i ];
            flagMap[ commandFlag.name ] = flagValue;
        }
    };

    return CommandLineOptions( programName, flagMap );
}

} // namespace commandLineParser
