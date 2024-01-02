#pragma once

#include <map>
#include <optional>
#include <string>

namespace commandLineParser {

struct CommandFlag {
    std::string name;
    std::string value;
};

class CommandLineOptions {
  public:
    CommandLineOptions( std::string programName,
                        std::map< std::string, std::string > flagMap )
        : mProgramName( programName ), mFlagMap( flagMap ) {}

    std::optional< std::string > query( std::string flagName ) {
        if ( mFlagMap.contains( flagName ) ) {
            return mFlagMap[ flagName ];
        } else {
            return std::nullopt;
        }
    }

    std::string programName() {
        return mProgramName;
    }

  private:
    std::map< std::string, std::string > mFlagMap;

    std::string mProgramName;
};

class CommandLineParser {
  public:
    /// Add flag
    void addFlag( std::string name, std::string shortName,
                  std::string defaultValue ) {
        mFlagNameToDefault[ name ] =
            CommandFlag{ .name = name, .value = defaultValue };
        mFlagShortNameToDefault[ shortName ] =
            CommandFlag{ .name = name, .value = defaultValue };
    }

    /// Parse the command line arguments
    /// @returns a flag to value map structure
    /// @see FlagsAndValues
    CommandLineOptions parse( int argc, char ** argv );

  private:
    std::map< std::string, CommandFlag > mFlagNameToDefault;
    std::map< std::string, CommandFlag > mFlagShortNameToDefault;
};

} // namespace commandLineParser
