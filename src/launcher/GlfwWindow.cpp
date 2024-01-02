#include "GlfwWindow.h"

// clang-format off

#define GLFW_INCLUDE_NONE
//#define GLFW_EXPOSE_NATIVE_WIN32
//#define GLFW_EXPOSE_NATIVE_WGL

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glad/glad.h>

// clang-format on

#include <iostream>
#include <stdexcept>

#include "Logging.h"

namespace engine {

static void printError( int error, const char * msg ) {
    std::cerr << "[GLFW Error " << error << "]: " << msg << std::endl;
}

GlfwWindow::GlfwWindow( const std::string & title, int width, int height ) {
    glfwSetErrorCallback( printError );

    glfwInit();

    // set some hints so that my window manager will float this window
    glfwWindowHintString( GLFW_X11_CLASS_NAME, "floater" );
    glfwWindowHintString( GLFW_X11_INSTANCE_NAME, "floater" );

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    mWindow =
        glfwCreateWindow( width, height, title.c_str(), nullptr, nullptr );
    if ( !mWindow ) {
        glfwTerminate();
        throw std::runtime_error( "Could not create window." );
    }
    glfwMakeContextCurrent( mWindow );

    if ( !gladLoadGLLoader( (GLADloadproc) glfwGetProcAddress ) ) {
        glfwTerminate();
        throw std::runtime_error( "Could load OpenGL context." );
    }

    DEBUG_LOG() << "OpenGL context and window created." << std::endl;
}

GlfwWindow::~GlfwWindow() {
    glfwDestroyWindow( mWindow );
    glfwTerminate();
    DEBUG_LOG() << "OpenGL context and window destroyed." << std::endl;
}

// Win32GLContext GlfwWindow::win32GLContext() {
//     Win32GLContext context;
//
//     context.hDC = GetDC( glfwGetWin32Window( mWindow ) );
//     context.hGLRC = glfwGetWGLContext( mWindow );
//
//     return context;
// }

} // namespace engine
