#include "GlfwWindow.hpp"

#include "Destroyer.hpp"
#include "ErrorString.hpp"
#include "Mutexed.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <optional>
#include <thread>

namespace
{
void throwGlfwErrorAsString( int error, const char *description )
{
  throw ErrorString( "GLFW error ", error, ": ", description );
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

  enum class RenderThreadState { shouldWait, shouldRender, shouldQuit };
  struct FrameSize { int width, height; };
  struct RenderThreadShared
  {
    RenderThreadState state = RenderThreadState::shouldRender;
    std::optional< FrameSize > frameSizeUpdate;
    std::function< void( void ) > fnRender;
  };
  Mutexed< RenderThreadShared > renderThreadShared;
  std::thread renderThread;

  std::future< makeGlRenderer_t > futureMakeGlRenderer;
  std::function< void() > onRender;

  std::unique_ptr< IGlRenderer > renderer;

  //------------------------------------------------------------------------------

  static void
  _glfwFramebufferSizeCallback( GLFWwindow *window, int width, int height )
  {
    auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
    gw->renderThreadShared.withLock(
        [=]( RenderThreadShared &rts ) { rts.frameSizeUpdate = { width, height }; } );
  }

  static void
  _glfwWindowRefreshCallback( GLFWwindow *window )
  {
    auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
    gw->renderThreadShared.withLockThenNotify(
        [=]( RenderThreadShared &rts ) { rts.state = RenderThreadState::shouldRender; } );
  }

  static void
  startGlew( GLFWwindow *window )
  {
    glfwMakeContextCurrent( window );

    if( GLEW_OK != glewInit())
      throw ErrorString( "glewInit() failed" );
  }

  //------------------------------------------------------------------------------

  void createGlfwWindow()
  {
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
//    glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
    glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );

    window = glfwCreateWindow( 640, 480, "", nullptr, nullptr );
    if( !window )
      throw ErrorString( "glfwCreateWindow(..) failed" );

    destroyers.glfwCreateWindow = { [window = this->window] { glfwDestroyWindow( window ); }};

    glfwSetWindowUserPointer( window, this );
    glfwSetFramebufferSizeCallback( window, _glfwFramebufferSizeCallback );
    glfwSetWindowRefreshCallback( window, _glfwWindowRefreshCallback );
  }

  void startGlfw()
  {
    if( !glfwInit())
      throw ErrorString( "glfwInit() failed" );

    destroyers.glfwInit = { glfwTerminate };

    glfwSetErrorCallback( throwGlfwErrorAsString );
  }

  void startRenderThread()
  {
    // deactivate the OpenGL context in the current thread
    // before making it active in the render thread, below
    glfwMakeContextCurrent( nullptr );

    renderThread = std::thread{
        [this]
        {
          // activate the OpenGL context in this thread (the render thread);
          // however it is now unusable in the original thread
          glfwMakeContextCurrent( this->window );

          auto waitPredicate =
              []( const RenderThreadShared &rts ) { return rts.state != RenderThreadState::shouldWait; };

          bool shouldQuit{};
          auto whileLocked =
              [&shouldQuit, this]( RenderThreadShared &rts )
              {
                if( rts.state == RenderThreadState::shouldQuit )
                {
                  shouldQuit = true;
                  return;
                }

                if( rts.frameSizeUpdate )
                {
                  auto[width, height] = *std::exchange( rts.frameSizeUpdate, std::nullopt );
                  glViewport( 0, 0, width, height );
                }

                if( rts.fnRender )
                  rts.fnRender();

                glfwSwapBuffers( this->window );

                rts.state = RenderThreadState::shouldWait;
              };

          while( !shouldQuit )
            renderThreadShared.waitThen( waitPredicate, whileLocked );
        }};
  }

  void stopRenderThread()
  {
    renderThreadShared.withLockThenNotify(
        []( RenderThreadShared &rts ) { rts.state = RenderThreadState::shouldQuit; } );

    if( renderThread.joinable())
      renderThread.join();
  }

  void unpackFutureMakeGlRenderer()
  {
    if( std::future_status::ready != futureMakeGlRenderer.wait_for( std::chrono::milliseconds( 0 )))
      return;

    const makeGlRenderer_t makeGlRenderer = futureMakeGlRenderer.get();
    futureMakeGlRenderer = {};

    renderer = (*makeGlRenderer)();
    onRender = [this] { this->renderer->render(); };
  }

  //------------------------------------------------------------------------------

  GlfwWindow(
      std::future< makeGlRenderer_t > futureMakeGlRenderer )
      : futureMakeGlRenderer{ std::move( futureMakeGlRenderer ) }
  {
    startGlfw();
    createGlfwWindow();
    startGlew( window );
    glfwSwapInterval( 0 );
    onRender = [this] { this->unpackFutureMakeGlRenderer(); };
  }

  virtual ~GlfwWindow() override
  {
    stopRenderThread();
  }

  //------------------------------------------------------------------------------

  virtual void
  enterEventLoop()
  override
  {
    startRenderThread();

    while( !glfwWindowShouldClose( window ))
      glfwWaitEvents();
  }

  virtual XYf
  getContentScale()
  override
  {
    XYf xyf;
    glfwGetWindowContentScale( window, &xyf.x, &xyf.y );
    return xyf;
  }

  virtual void
  hide()
  override { glfwHideWindow( window ); }

  virtual void
  setContentAspectRatio( int numer, int denom )
  override { glfwSetWindowAspectRatio( window, numer, denom ); }

  virtual void
  setContentSize( int width, int height )
  override { glfwSetWindowSize( window, width, height ); }

  virtual void
  setRenderFunction( std::function< void( void ) > fnRender )
  override
  {
    renderThreadShared.withLock(
        [&]( RenderThreadShared &rts ) { rts.fnRender = std::move( fnRender ); } );
  }

  virtual void
  setTitle( const char *title )
  override { glfwSetWindowTitle( window, title ); }

  virtual void
  show()
  override { glfwShowWindow( window ); }
};
}

std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< makeGlRenderer_t > futureMakeGlRenderer )
{
  return std::make_unique< GlfwWindow >( std::move( futureMakeGlRenderer ));
}
