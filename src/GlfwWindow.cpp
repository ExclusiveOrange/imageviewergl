#include "GlfwWindow.hpp"

#include "Destroyer.hpp"
#include "toString.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace
{

void throwGlfwErrorAsString( int error, const char *description )
{
  throw std::runtime_error( toString( "GLFW error: ", error, ": ", description ));
}

void _glfwFrameSizeCallback( GLFWwindow *window, int width, int height )
{
  std::unique_lock lk( renderThreadShared.mutex );
  renderThreadShared.frameSizeUpdate = { width, height };
//  renderThreadShared.state = RenderThreadState::shouldRender;
//  renderThreadShared.condVar.notify_one();
}

void _glfwWindowRefreshCallback( GLFWwindow *window )
{
  GlfwWindow *gw = glfwGetWindowUserPointer( window );
  std::unique_lock lk( renderThreadShared.mutex );
  renderThreadShared.state = RenderThreadState::shouldRender;
  renderThreadShared.condVar.notify_one();
}

//==============================================================================

struct GlfwWindow : public IGlWindow
{
  GlfwWindow()
  {
    glfwSetErrorCallback( throwGlfwErrorAsString );

  }


  ~GlfwWindow()
  {
    // TODO
  }
};
}

std::unique_ptr< IGlWindow >
makeGlfwWindow()
{
  return std::make_unique< GlfwWindow >();
}
