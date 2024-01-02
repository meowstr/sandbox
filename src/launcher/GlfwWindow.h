#pragma once

//#include <windows.h>

#include <string>

class GLFWwindow;

namespace engine {

//struct Win32GLContext {
//    HDC hDC;
//    HGLRC hGLRC;
//};

class GlfwWindow {
  public:
    GlfwWindow( const std::string & title, int width, int height );
    ~GlfwWindow();

    GLFWwindow * get() {
        return mWindow;
    }

    //Win32GLContext win32GLContext();

    operator GLFWwindow *() const {
        return mWindow;
    }

  private:
    GLFWwindow * mWindow;
};

} // namespace engine
