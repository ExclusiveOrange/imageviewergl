#include "GlfwWindow.hpp"

#include "Destroyer.hpp"
#include "ErrorString.hpp"
#include "Mutexed.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <optional>
#include <thread>
#include <vector>

namespace
{
  void throwGlfwErrorAsString( int error, const char *description )
  {
    throw ErrorString( "GLFW error ", error, ": ", description );
  }

//==============================================================================

  struct GlfwWindow : public IGlWindow, public IGlWindowAppearance
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

      std::future< std::unique_ptr< IGlRendererMaker >> futureGlRendererMaker;
      std::unique_ptr< IGlRenderer > renderer;

      std::vector< std::function< void() >> runInMainThread;
    };
    Mutexed< RenderThreadShared > renderThreadShared;
    std::thread renderThread;

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

    void enqueueForMainThread( std::function< void() > f )
    {
      renderThreadShared.withoutLock(
          [f = std::move( f )]( RenderThreadShared &rts )
          {
            rts.runInMainThread.emplace_back( std::move( f ));
          } );
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

                  if( !rts.renderer )
                    if( std::future_status::ready
                        == rts.futureGlRendererMaker.wait_for( std::chrono::milliseconds( 0 )))
                    {
                      const std::unique_ptr< IGlRendererMaker > rendererMaker = rts.futureGlRendererMaker.get();
                      rts.futureGlRendererMaker = {};
                      rts.renderer = rendererMaker->makeGlRenderer( *this );
                    }

                  if( rts.frameSizeUpdate )
                  {
                    auto[width, height] = *std::exchange( rts.frameSizeUpdate, std::nullopt );
                    glViewport( 0, 0, width, height );
                  }

                  if( rts.renderer )
                    rts.renderer->render();

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

    //------------------------------------------------------------------------------

    GlfwWindow(
        std::future< std::unique_ptr< IGlRendererMaker >> futureGlRendererMaker )
    {
      startGlfw();
      createGlfwWindow();
      startGlew( window );
      glfwSwapInterval( 0 );

      renderThreadShared.withLock(
          [&]( RenderThreadShared &rts )
          {
            rts.futureGlRendererMaker = std::move( futureGlRendererMaker );
          } );
    }

    virtual ~GlfwWindow() override
    {
      stopRenderThread();
    }

    //------------------------------------------------------------------------------
    // IGlWindow overrides

    virtual void
    enterEventLoop()
    override
    {
      startRenderThread();

      while( !glfwWindowShouldClose( window ))
      {
        glfwWaitEvents();

        std::vector< std::function< void() >> runInMainThread;
        renderThreadShared.withLock(
            [&runInMainThread]( RenderThreadShared &rts )
            {
              if( !rts.runInMainThread.empty())
                runInMainThread.swap( rts.runInMainThread );
            } );

        for( auto &fn : runInMainThread )
          fn();
      }
    }

    virtual void
    hide()
    override { glfwHideWindow( window ); }

    virtual void
    show()
    override { glfwShowWindow( window ); }

    //------------------------------------------------------------------------------
    // IGlWindowAppearance overrides

    virtual XYf
    getContentScale()
    override
    {
      XYf xyf;
      glfwGetWindowContentScale( window, &xyf.x, &xyf.y );
      return xyf;
    }

    virtual void
    setContentAspectRatio( int numer, int denom )
    override
    {
      enqueueForMainThread( std::bind( glfwSetWindowAspectRatio, window, numer, denom ));
      glfwPostEmptyEvent();
    }

    virtual void
    setContentSize( int width, int height )
    override
    {
      enqueueForMainThread( std::bind( glfwSetWindowSize, window, width, height ));
      glfwPostEmptyEvent();
    }

    virtual void
    setTitle( std::string title )
    override
    {
      enqueueForMainThread(
          [=, title = std::move( title )]
          {
            glfwSetWindowTitle( window, title.c_str());
          } );
      glfwPostEmptyEvent();
    }
  };
}

std::unique_ptr< IGlWindow >
makeGlfwWindow(
    std::future< std::unique_ptr< IGlRendererMaker >> futureGlRendererMaker )
{
  return std::make_unique< GlfwWindow >( std::move( futureGlRendererMaker ));
}
