#include "GameRegistry.h"

#include <fstream>
#include <stdexcept>

#include <dlfcn.h>

#include "Logging.h"

namespace game {

static IGame * gGame;      // nullptr
static void * gGameHandle; // nullptr

static void copyFile( std::string destName, std::string srcName ) {
    std::ifstream src( srcName, std::ios::binary );
    std::ofstream dst( destName, std::ios::binary );

    dst << src.rdbuf();
}

/// The global game interface
IGame & binds() {
    if ( !gGame ) {
        throw std::runtime_error( "No game bindings, make sure the game shared "
                                  "library is loaded properly." );
    }

    return *gGame;
}

void bindFunctions( IGame * game ) {
    DEBUG_LOG() << "Binding game functions." << std::endl;

    if ( !game ) {
        throw std::runtime_error( "Invalid game bindings (NULL bindings)." );
    }

    gGame = game;
}

void unbindFunctions() {
    gGame = nullptr;
}

void loadGameBinds( std::string name ) {
    if ( gGameHandle )
        unloadGameBinds();

    static int tempIndex = 0;
    std::string tempName = "/tmp/hot." + std::to_string( tempIndex ) + "." + name;
    copyFile( tempName, name );

    DEBUG_LOG() << "Loading game binds (" << tempName << ")." << std::endl;

    tempIndex++;

    gGameHandle = dlopen( tempName.c_str(), RTLD_NOW );

    if ( !gGameHandle ) {
        throw std::runtime_error( dlerror() );
    }

    using InitFunction = void ( * )();

    auto init = (InitFunction) dlsym( gGameHandle, "initSharedLibrary" );

    if ( const char * error = dlerror() ) {
        throw std::runtime_error( dlerror() );
    }

    init();
}

void unloadGameBinds() {
    if ( gGameHandle ) {
        DEBUG_LOG() << "Unloading game binds." << std::endl;
        unbindFunctions();

        if ( dlclose( gGameHandle ) ) {
            ERROR_LOG() << "Failed to unload." << std::endl;
        }

        gGameHandle = nullptr;
    }
}

} // namespace game
