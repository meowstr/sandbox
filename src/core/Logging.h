#pragma once

#include <sstream>

namespace logging {

void logInfo( const std::string & );
void logDebug( const std::string & );
void logError( const std::string & );

#define INFO_LOG() logging::Logger( logging::logInfo ).stream()
#define DEBUG_LOG() logging::Logger( logging::logDebug ).stream()
#define ERROR_LOG() logging::Logger( logging::logError ).stream()

class Logger {
  public:
    using LogFunction = void( const std::string & );
    Logger( LogFunction * f, const char * startText = nullptr ) : mF( f ) {
        if ( startText ) {
            mStream << startText;
        }
    }

    std::ostringstream & stream() {
        return mStream;
    }

    ~Logger() {
        mF( mStream.str() );
    }

  private:
    std::ostringstream mStream;
    LogFunction * mF;
};

}; // namespace logging

#define LOGGER_ASSERT( expr )                                                  \
    if ( !( expr ) ) {                                                         \
        ERROR_LOG() << "Assert failed: " << #expr << " at " << __FILE__ << ":" \
                    << __LINE__ << "\n";                                       \
    }
