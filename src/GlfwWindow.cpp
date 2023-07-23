#include "GlfwWindow.hpp"

#include "Destroyer.hpp"
#include "ErrorString.hpp"
#include "GlWindowInputHandler.hpp"
#include "Mutexed.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <optional>
#include <thread>

/* TODO
   
glfwGetWindowFrameSize returns the wrong values on Windows 10 (maybe on 11 too).
This makes it unusable for centering the window and constraining the maximimum size.
So I have turned off window "decorations", and now the frame size is 0,0,0,0, which is good for visibility but bad for usability.
Now I will have to manually add back in window dragging and resizing and closing (important!).
Closing could be done with the Esc key at first, but eventually I'd like a visual X on the top right (or somewhere obvious) on hover in that area.
Sizing isn't terribly important yet, but moving the window around is important and could be done with simple left-click drag.

*/ 

// DELETE
#include <iostream>

namespace
{
  void
  startGlew( GLFWwindow *window )
  {
    glfwMakeContextCurrent( window );

    if( GLEW_OK != glewInit())
      throw ErrorString( "glewInit() failed" );
  }

  void throwGlfwErrorAsString( int error, const char *description )
  {
    throw ErrorString( "GLFW error ", error, ": ", description );
  }

//==============================================================================

  struct WindowState
  {
    struct
    {
      Destroyer
          glfwInit,
          glfwCreateWindow;
    } destroyers;

    GLFWwindow *window{};

    enum class RenderThreadState { shouldWait, shouldRender, shouldQuit };
    struct RenderThreadShared
    {
      RenderThreadState state = RenderThreadState::shouldRender;

      struct FrameSize { int width, height; };
      std::optional<FrameSize> frameSizeUpdate;

      std::future<std::unique_ptr<IGlRendererMaker >> futureGlRendererMaker;
      std::unique_ptr<IGlRenderer> renderer;
    };
    Mutexed<RenderThreadShared> renderThreadShared;
    std::thread renderThread;

    std::shared_ptr<GlWindowInputHandler> inputHandler
        { std::make_shared<GlWindowInputHandler>() }; // initialize with a default value so we don't have to check validity later
  };

//==============================================================================

  struct GlfwWindow : IGlWindow, WindowState
  {
    struct GlfwRenderCallbacks
    {
      static void
      framebufferSize( GLFWwindow *window, int width, int height )
      {
        auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
        gw->renderThreadShared.withLock(
            [=]( RenderThreadShared &rts ) { rts.frameSizeUpdate = { width, height }; } );
      }

      static void
      windowRefresh( GLFWwindow *window )
      {
        auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
        gw->renderThreadShared.withLockThenNotify(
            [=]( RenderThreadShared &rts ) { rts.state = RenderThreadState::shouldRender; } );
      }
    };

    //------------------------------------------------------------------------------

    struct GlfwInputCallbacks
    {
      static void
      scroll( GLFWwindow *window, double xoffset, double yoffset )
      {
        auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
        gw->inputHandler->onScroll( xoffset, yoffset );
      }

      static void
      cursorPosition( GLFWwindow *window, double xpos, double ypos )
      {
        auto gw = static_cast< GlfwWindow * >( glfwGetWindowUserPointer( window ));
        gw->inputHandler->onCursorPosition( xpos, ypos );
      }
    };

    //------------------------------------------------------------------------------

    void createGlfwWindow()
    {
      glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
      glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
      glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
      glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
      glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
      glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
      glfwWindowHint( GLFW_DECORATED, GLFW_FALSE );

      window = glfwCreateWindow( 640, 480, "", nullptr, nullptr );
      if( !window )
        throw ErrorString( "glfwCreateWindow(..) failed" );

      destroyers.glfwCreateWindow = Destroyer{ [window = this->window] { glfwDestroyWindow( window ); }};

      glfwSetWindowUserPointer( window, this );

      // glfw callbacks involving render thread
      glfwSetFramebufferSizeCallback( window, GlfwRenderCallbacks::framebufferSize );
      glfwSetWindowRefreshCallback( window, GlfwRenderCallbacks::windowRefresh );

      // glfw input callbacks (not render thread)
      glfwSetScrollCallback( window, GlfwInputCallbacks::scroll );
      glfwSetCursorPosCallback( window, GlfwInputCallbacks::cursorPosition );
    }

    void startGlfw()
    {
      if( !glfwInit())
        throw ErrorString( "glfwInit() failed" );

      destroyers.glfwInit = Destroyer{ glfwTerminate };

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
            
            // wait for renderer to exist
            renderThreadShared.withLock(
              []( RenderThreadShared &rts ){ rts.renderer = rts.futureGlRendererMaker.get()->makeGlRenderer(); }
            );

            auto waitPredicate =
                []( const RenderThreadShared &rts ) { return rts.state != RenderThreadState::shouldWait; };

            auto whileLocked =
                [this]( RenderThreadShared &rts )
                    -> bool // true to continue, false to quit
                {
                  if( rts.state == RenderThreadState::shouldQuit )
                    return false;

                  if( rts.frameSizeUpdate )
                  {
                    auto [width, height] = *std::exchange(rts.frameSizeUpdate, std::nullopt);
                    glViewport( 0, 0, width, height );
                  }

                  rts.renderer->render();

                  glfwSwapBuffers( this->window );

                  rts.state = RenderThreadState::shouldWait;

                  return true;
                };

            while( renderThreadShared.waitThen( waitPredicate, whileLocked ));
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

    explicit
    GlfwWindow(
        std::future<std::unique_ptr<IGlRendererMaker >>
        futureGlRendererMaker )
    {
      startGlfw();
      createGlfwWindow();
      startGlew( window );
      glfwSwapInterval( 0 );

      renderThreadShared.withLock(
          [&]( RenderThreadShared &rts ) { rts.futureGlRendererMaker = std::move( futureGlRendererMaker ); } );
    }

    ~GlfwWindow()
    override
    {
      stopRenderThread();
    }

//------------------------------------------------------------------------------
// IGlWindow overrides

    void
    enterEventLoop( std::shared_ptr<GlWindowInputHandler> _inputHandler )
    override
    {
      if( _inputHandler )
        this->inputHandler = std::move( _inputHandler );

      startRenderThread();

      while( !glfwWindowShouldClose( window ))
        glfwWaitEvents();
    }

    void
    hide()
    override
    {
      glfwHideWindow( window );
    }

    void
    show()
    override
    {
      glfwShowWindow( window );
    }

//------------------------------------------------------------------------------
// IGlWindowAppearance overrides

    XYf
    getContentScale()
    override
    {
      XYf xyf{};
      glfwGetWindowContentScale( window, &xyf.x, &xyf.y );
      return xyf;
    }

    void
    setCenteredToFit( int contentWidth, int contentHeight ) 
    override
    {
      // get monitor work area
      // TODO: figure out which monitor the center of the window is in and use that one instead of necessarily the primary monitor
      GLFWmonitor *monitor = glfwGetPrimaryMonitor();
      int xpos = 0, ypos = 0, width = 0, height = 0;
      glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);
      std::cout << "xpos: " << xpos << ", ypos: " << ypos << ", width: " << width << ", height: " << height << std::endl;

      // get window frame size
      int left = 0, top = 0, right = 0, bottom = 0;
      glfwGetWindowFrameSize( window, &left, &top, &right, &bottom );
      std::cout << "left: " << left << ", top: " << top << ", right: " << right << ", bottom: " << bottom << std::endl;

      const int availWidth = width - left - right;
      const int availHeight = height - top - bottom;

      std::cout << "availWidth: " << availWidth << ", availHeight: " << availHeight << std::endl;
      
      std::cout << "contentWidth: " << contentWidth << ", contentHeight: " << contentHeight << std::endl;

      if (contentWidth <= availWidth && contentHeight <= availHeight) {
        // set the window content size directly and then center the window within the work area
        glfwSetWindowSize( window, contentWidth, contentHeight );
        const int centeredX = xpos + left + (availWidth / 2) - (contentWidth / 2);
        const int centeredY = ypos + top + (availHeight / 2) - (contentHeight / 2);
        glfwSetWindowPos( window, centeredX, centeredY );
      }
      else {
        // TODO: content is larger than available space, so constrain while maintaining aspect ratio
      }
      
    }

    void
    setContentAspectRatio( int numer, int denom )
    override
    {
      glfwSetWindowAspectRatio( window, numer, denom );
    }

    void
    setContentSize( int width, int height )
    override
    {
      glfwSetWindowSize( window, width, height );
    }

    void setTitle( const std::string &title )
    override
    {
      glfwSetWindowTitle( window, title.c_str());
    }
  };
}

std::unique_ptr<IGlWindow>
makeGlfwWindow(
    std::future<std::unique_ptr<IGlRendererMaker >> futureGlRendererMaker )
{
  return std::make_unique<GlfwWindow>( std::move( futureGlRendererMaker ));
}
