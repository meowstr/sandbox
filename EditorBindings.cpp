#include "Game.h"
#include "Logging.h"
#include "Map.h"

#include "ResourceDirectory.h"
#include "Resources.h"

#include <glm/gtx/transform.hpp>

namespace {

class EditorController : public Controller {
  public:
    void setEvents( ControllerEvents & events ) override {
        mEvents = &events;
    }

    ControllerEvents & events() {
        if ( !mEvents ) {
            throw std::runtime_error( "Event handler not registered yet." );
        }
        return *mEvents;
    }

  private:
    ControllerEvents * mEvents = nullptr;
};

std::unique_ptr< Game > gGame;
EditorController gController;

int gWidth = 800;
int gHeight = 600;

} // namespace

extern "C" void init() {
    INFO_LOG() << "Initializing" << std::endl;

    if ( !gladLoadGL() ) {
        throw std::runtime_error( "Could not load OpenGL context." );
    }

    // load resources
    res::loadResources();

    gGame.reset( new Game( gController ) );
}

extern "C" void render() {
    if ( gGame ) {
        Shiro shiro;
        shiro.projectionMatrix =
            glm::perspectiveFov( glm::radians( 45.0f ), (float) gWidth,
                                 (float) gHeight, 0.01f, 100.0f );

        shiro.viewMatrix = glm::mat4( 1.0f );
        // gGame->render( shiro, 1.0f / 60.0f );
        gGame->render( shiro, 0.0f );
    }
}

extern "C" void resize( int width, int height ) {
    INFO_LOG() << "Resizing" << std::endl;
    gWidth = width;
    gHeight = height;

    if ( gGame ) {
        gController.events().setSize( width, height );
    }
}

extern "C" void loadMap( const Map * map ) {
    INFO_LOG() << "Loading map" << std::endl;

    debugPrintMap( map );
}

extern "C" void dispose() {
    INFO_LOG() << "Disposing" << std::endl;

    res::unloadResources();
    gGame.reset( nullptr );
}

extern "C" void doEditorAction( int action, void * data ) {
    if (action == 1) {
    }
}

extern "C" MapProperty * prefabs( int * oPrefabCount ) {
    INFO_LOG() << "Fetching prefabs" << std::endl;

    oPrefabCount = 0;
    return nullptr;
}

extern "C" Map * defaultMap() {
    INFO_LOG() << "Fetching default map" << std::endl;

    static std::unique_ptr< utils::MapRaii > map;

    if ( !map ) {
        std::string data =
            utils::stringFromFile( res::directory() / "maps/default.xml" );
        map = std::make_unique< utils::MapRaii >( data.data() );
        debugPrintMap( map->get() );
    }

    return map->get();
}
