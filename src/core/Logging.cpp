#include "Logging.h"

#include <rang.hpp>

#include <iostream>

namespace logging {

void logInfo( const std::string & str ) {
    std::cout << "[" << rang::style::bold << rang::fg::green << " INFO  "
              << rang::style::reset << "] " << str;
}

void logDebug( const std::string & str ) {
    std::cout << "[" << rang::style::bold << rang::fg::magenta << " DEBUG "
              << rang::style::reset << "] " << str;
}

void logError( const std::string & str ) {
    std::cout << "[" << rang::style::bold << rang::fg::red << " ERROR "
              << rang::style::reset << "] " << str;
}

}; // namespace logging
