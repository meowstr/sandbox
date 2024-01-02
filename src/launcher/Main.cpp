#include "CommandLineParser.h"
#include "DesktopApp.h"
#include "Logging.h"

#include <iostream>
#include <memory>

enum class AppType {
    Desktop,
    Vr,
};

struct AppConfig {
    AppType type = AppType::Desktop;
};

AppConfig parseArgs( int argc, char ** argv ) {
    commandLineParser::CommandLineParser parser;

    parser.addFlag( "apptype", "t", "desktop" );
    auto options = parser.parse( argc, argv );

    AppConfig config;

    std::string appTypeString = *options.query( "apptype" );
    if ( appTypeString == "desktop" ) {
        config.type = AppType::Desktop;
    } else if ( appTypeString == "vr" ) {
        config.type = AppType::Vr;
    } else {
        ERROR_LOG() << "Invalid applciation type: " << appTypeString
                    << std::endl;
    }

    return config;
}

int main( int argc, char ** argv ) {

    std::unique_ptr< App > app;

    AppConfig config;

    try {
        config = parseArgs( argc, argv );
    } catch ( const std::exception & e ) {
        ERROR_LOG() << "Could not parse command line args: " << e.what()
                    << std::endl;
        return 1;
    }

    switch ( config.type ) {
    case AppType::Desktop:
        app = std::make_unique< DesktopApp >();
        break;
    case AppType::Vr:
        // app = std::make_unique< VrApp >();
        return 1;
    }

    try {
        app->exec();
    } catch ( const std::exception & e ) {
        ERROR_LOG() << e.what() << std::endl;
        return 1;
    }

    INFO_LOG() << "Goodbye :3" << std::endl;

    return 0;
}
