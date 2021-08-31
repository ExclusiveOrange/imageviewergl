#include "GlfwWindow.hpp"

#include "Destroyer.hpp"
#include "ErrorString.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

namespace
{
void throwGlfwErrorAsString( int error, const char *description )
{
  throw ErrorString( "GLFW error: ", error, ": ", description );
}

//==============================================================================

struct GlfwWindow : public IGlWindow
{
  struct
  {
    Destroyer
        glfwInit,
        glfwCreateWindow;
  } destroyers;

  GLFWwindow *window{};

  //------------------------------------------------------------------------------

  static void
  _glfwFramebufferSizeCallback( GLFWwindow *window, int width, int height )
  {
    // TODO
    std::unique_lock lk( renderThreadShared.mutex );
    renderThreadShared.frameSizeUpdate = { width, height };
//  renderThreadShared.state = RenderThreadState::shouldRender;
//  renderThreadShared.condVar.notify_one();
  }

  static void
  _glfwWindowRefreshCallback( GLFWwindow *window )
  {
    GlfwWindow *gw = glfwGetWindowUserPointer( window );
    std::unique_lock lk( renderThreadShared.mutex );
    renderThreadShared.state = RenderThreadState::shouldRender;
    renderThreadShared.condVar.notify_one();
  }

  //------------------------------------------------------------------------------

  GlfwWindow()
  {
    glfwSetErrorCallback( throwGlfwErrorAsString );

    if( !glfwInit())
      throw ErrorString( "glfwInit() failed" );
    destroyers.glfwInit = glfwTerminate;

    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
    glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
    glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );
    window = glfwCreateWindow( 640, 480, "My Title", nullptr, nullptr );
    if( !window )
      throw ErrorString( "glfwCreateWindow(..) failed" );
    destroyers.glfwCreateWindow = [window] { glfwDestroyWindow( window ); };

    glfwSetWindowUserPointer( window, this );
    glfwSetFramebufferSizeCallback( window, _glfwFramebufferSizeCallback );
    glfwSetWindowRefreshCallback( window, _glfwWindowRefreshCallback );

    glfwMakeContextCurrent( window );
    if( GLEW_OK != glewInit())
      throw ErrorString( "glewInit() failed" );
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
