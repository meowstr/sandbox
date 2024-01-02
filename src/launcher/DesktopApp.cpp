#include "DesktopApp.h"

#include "FileMonitor.h"
#include "Game.h"
#include "GameRegistry.h"
#include "GlfwWindow.h"
#include "ResourceDirectory.h"
#include "SharedState.h"

#include "Logging.h"

// clang-format off

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

// clang-format on

#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

static const int kInitialWindowWidth = 1600;
static const int kInitialWindowHeight = 1200;

namespace {

static float gLastTimestep = 0.0f;
// static engine::GlfwController * gController = nullptr;

static state::GameState * gState;
static int gWindowWidth = kInitialWindowWidth;
static int gWindowHeight = kInitialWindowHeight;

static bool pendingReloadRequest = false;

// class GlfwGameController : public Controller {
//   public:
//     GlfwGameController( GLFWwindow * window ) : mWindow( window ) {}
//
//     bool run() override {
//         return glfwGetKey( mWindow, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS;
//     }
//
//     bool right() override {
//         return glfwGetKey( mWindow, GLFW_KEY_D ) == GLFW_PRESS;
//     }
//
//     bool left() override {
//         return glfwGetKey( mWindow, GLFW_KEY_A ) == GLFW_PRESS;
//     }
//
//     bool forward() override {
//         return glfwGetKey( mWindow, GLFW_KEY_W ) == GLFW_PRESS;
//     }
//
//     bool backward() override {
//         return glfwGetKey( mWindow, GLFW_KEY_S ) == GLFW_PRESS;
//     }
//
//     bool jump() override {
//         return glfwGetKey( mWindow, GLFW_KEY_SPACE ) == GLFW_PRESS;
//     }
//
//     bool mouseDown() override {
//         return glfwGetMouseButton( mWindow, GLFW_MOUSE_BUTTON_LEFT ) ==
//                GLFW_PRESS;
//     }
//
//     float yaw() override {
//         double x, y;
//         glfwGetCursorPos( mWindow, &x, &y );
//
//         return x * mouseSensitivity;
//     }
//
//     float pitch() override {
//         double x, y;
//         glfwGetCursorPos( mWindow, &x, &y );
//
//         return -y * mouseSensitivity;
//     }
//
//     float mouseX() override {
//         double x, y;
//         glfwGetCursorPos( mWindow, &x, &y );
//
//         return x;
//     }
//
//     float mouseY() override {
//         double x, y;
//         glfwGetCursorPos( mWindow, &x, &y );
//
//         return y;
//     }
//
//     void setEvents( ControllerEvents & events ) override {
//         mEvents = &events;
//     }
//
//     ControllerEvents & events() {
//         if ( !mEvents ) {
//             throw std::runtime_error( "Event handler not registered yet." );
//         }
//         return *mEvents;
//     }
//
//   private:
//     float mouseSensitivity = 0.001f;
//     GLFWwindow * mWindow;
//
//     ControllerEvents * mEvents = nullptr;
// };

void printFps() {
    if ( gLastTimestep == 0.0f )
        return;

    // INFO_LOG() << "FPS: " << ( 1.0f / gLastTimestep ) << std::endl;
}

void handleWindowResize( GLFWwindow * window, int width, int height ) {
    glViewport( 0, 0, width, height );

    gWindowWidth = width;
    gWindowHeight = height;

    game::InputEventData eData;
    eData.size[ 0 ] = width;
    eData.size[ 1 ] = height;
    game::binds().input( *gState, game::InputEvent::kResize, eData );
}

void handleMouseScroll( GLFWwindow * window, double xoffset, double yoffset ) {
    // if ( yoffset > 0.0 ) {
    //     gGameController->events().rotateEditTransform( 1 );
    // } else if ( yoffset < 0.0 ) {
    //     gGameController->events().rotateEditTransform( -1 );
    // }
}

void handleKeyPress( GLFWwindow * window, int key, int scancode, int action,
                     int mods ) {
    game::InputEventData eData;
    if ( key == GLFW_KEY_F1 && action == GLFW_PRESS ) {
        pendingReloadRequest = true;
    }

    if ( key == GLFW_KEY_R && action == GLFW_PRESS ) {
        game::binds().input( *gState, game::InputEvent::kRotate, eData );
    }

    // if ( key == GLFW_KEY_E && action == GLFW_PRESS ) {
    //     gGameController->events().toggleEditTransform();
    // }
    // if ( key == GLFW_KEY_Q && action == GLFW_PRESS ) {
    //     gGameController->events().cancelEditTransform();
    // }
}

void handleMouseButton( GLFWwindow * window, int button, int action,
                        int mods ) {

    // load mouse position into shared state
    auto & sharedState = game::binds().sharedState( *gState );
    double mousePos[ 2 ];
    glfwGetCursorPos( window, &mousePos[ 0 ], &mousePos[ 1 ] );

    sharedState.mouseCursor[ 0 ] = (int) mousePos[ 0 ];
    sharedState.mouseCursor[ 1 ] = (int) mousePos[ 1 ];

    // load mouse position into event data
    game::InputEventData eData;
    eData.size[ 0 ] = mousePos[ 0 ];
    eData.size[ 1 ] = mousePos[ 1 ];

    if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
        game::binds().input( *gState, game::InputEvent::kClick, eData );
    }
}

void registerCallbacks( engine::GlfwWindow & window ) {
    glfwSetWindowSizeCallback( window, handleWindowResize );
    glfwSetKeyCallback( window, handleKeyPress );
    glfwSetScrollCallback( window, handleMouseScroll );
    glfwSetMouseButtonCallback( window, handleMouseButton );
}

void tick( engine::GlfwWindow & window, float timestep ) {
    auto & sharedState = game::binds().sharedState( *gState );

    double mousePos[ 2 ];
    glfwGetCursorPos( window, &mousePos[ 0 ], &mousePos[ 1 ] );

    sharedState.mouseCursor[ 0 ] = (int) mousePos[ 0 ];
    sharedState.mouseCursor[ 1 ] = (int) mousePos[ 1 ];

    game::binds().tick( *gState, timestep );
}

void render( engine::GlfwWindow & window, float timestep ) {
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
             GL_STENCIL_BUFFER_BIT );

    Shiro shiro{ .viewMatrix = glm::mat4( 1.0f ),
                 .projectionMatrix = glm::perspectiveFov(
                     glm::radians( 45.0f ), (float) gWindowWidth,
                     (float) gWindowHeight, 0.01f, 100.0f ) };

    game::binds().render( *gState, shiro, timestep );

    Shiro shiroUi{ .viewMatrix = glm::mat4( 1.0f ),
                   .projectionMatrix =
                       glm::ortho( 0.0f, (float) gWindowWidth,
                                   (float) gWindowHeight, 0.0f ) };

    game::binds().renderUi( *gState, shiroUi, timestep );

    glfwSwapBuffers( window );

    gLastTimestep = timestep;
    // mFpsTimer.tick( timestep );
}

struct StatePtr {
    StatePtr() {
        ptr = game::binds().allocateState();
    }

    ~StatePtr() {
        game::binds().deallocateState( ptr );
    }

    state::GameState * ptr;
};

struct FileMonitorPtr {
    fileMonitor::Info * info;

    ~FileMonitorPtr() {
        fileMonitor::stop( *info );
    }
};

} // namespace

void DesktopApp::exec() {
    INFO_LOG() << "Hi desktop!" << std::endl;

    // build the window
    engine::GlfwWindow window( "Sussy baka", kInitialWindowWidth,
                               kInitialWindowHeight );

    game::loadGameBinds( "libgame.so" );

    fileMonitor::Info fileMonitorInfo;
    fileMonitorInfo.path = "./libgame.so";
    fileMonitor::start( fileMonitorInfo );
    FileMonitorPtr fileMonitorPtr{ .info = &fileMonitorInfo };

    fileMonitor::Info fileMonitorInfo2;
    fileMonitorInfo2.path = res::directory() / "shaders";
    fileMonitorInfo2.wholeDirectory = 1;
    fileMonitor::start( fileMonitorInfo2 );
    FileMonitorPtr fileMonitorPtr2{ .info = &fileMonitorInfo2 };

    fileMonitor::Info stateHookInfo;
    stateHookInfo.path = "./libstateHook.so";
    fileMonitor::start( stateHookInfo );
    FileMonitorPtr stateHookPtr{ .info = &stateHookInfo };

    StatePtr state;

    gState = state.ptr;

    registerCallbacks( window );

    // setup glfw
    // glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );

    if ( glfwRawMouseMotionSupported() ) {
        glfwSetInputMode( window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE );
    }

    game::binds().init( *gState );

    handleWindowResize( window, kInitialWindowWidth, kInitialWindowHeight );

    // glfwSwapInterval( 0 );
    const bool enableSpinWait = false;

    bool stateHookUpdated = false;

    using namespace std::chrono;
    // TODO: upgrade to fixed timestep loop for physics
    float lastFrameTime = glfwGetTime();
    float inputLag = 0.0f;
    float targetFrameLag = 1 / 60.0f;
    float lagWait = 0.0f;

    const float lagThreshold = 1 * 0.001f;
    while ( !glfwWindowShouldClose( window ) ) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;

        if ( fileMonitor::fileUpdated( stateHookInfo ) ) {
            stateHookUpdated = true;
        }

        if ( fileMonitor::fileUpdated( fileMonitorInfo ) ||
             fileMonitor::fileUpdated( fileMonitorInfo2 ) ||
             pendingReloadRequest ) {
            if ( stateHookUpdated || pendingReloadRequest ) {
                // deallocate old state before loading new destructors
                game::binds().deallocateState( state.ptr );
            }

            game::loadGameBinds( "libgame.so" );

            if ( stateHookUpdated || pendingReloadRequest ) {
                // full reset
                state.ptr = game::binds().allocateState();
                gState = state.ptr;
                game::binds().init( *gState );
                handleWindowResize( window, kInitialWindowWidth,
                                    kInitialWindowHeight );

                stateHookUpdated = false;
            }

            pendingReloadRequest = false;
        }

        if ( deltaTime > targetFrameLag + lagThreshold ) {
            // we're dropping frames, decrease wait
            lagWait -= 0.0001f;
        } else if ( inputLag > 4 * lagThreshold ) {
            // input lag is getting bad, increase wait
            lagWait += 0.0001f;
        }
        if ( lagWait < 0.0f )
            lagWait = 0.0f;

        if ( enableSpinWait ) {
            // auto t1 = high_resolution_clock::now();
            // while ( duration_cast< milliseconds >(
            //             high_resolution_clock::now() - t1 )
            //             .count() < 14 )
            //     ;

            float start = glfwGetTime();

            while ( glfwGetTime() - start < lagWait ) {
                // spinnnnnn
            }
        }

        float inputPollTimePoint = glfwGetTime();

        glfwPollEvents();

        if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) ) {
            glfwSetWindowShouldClose( window, true );
        }

        tick( window, deltaTime );
        render( window, deltaTime );

        // glFlush();
        // glFinish();

        float endOfFrameTimepoint = glfwGetTime();
        inputLag = endOfFrameTimepoint - inputPollTimePoint;

        // gameController.events().setInputLag( inputLag );

        lastFrameTime = currentTime;
    }
}
