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

  enum class RenderThreadState
  {
    shouldWait,
    shouldRender,
    shouldQuit
  };

//==============================================================================

  struct RenderThreadShared
  {
    RenderThreadState state = RenderThreadState::shouldRender;

    struct FrameSize
    {
      int width, height;
    };
    std::optional<FrameSize> frameSizeUpdate;

    std::future<std::unique_ptr<IGlRendererMaker>> futureGlRendererMaker;
    std::unique_ptr<IGlRenderer> renderer;
  };
  
//==============================================================================

  struct InputHandler : GlWindowInputHandler
  {
    InputHandler(IGlWindow &window) : window{window} {}

    void onCursorPosition(double xPos, double yPos) override
    {
      if (dragging)
        drag(xPos, yPos);
    }

    void onKeyDown(int key, int scancode, int mods) override
    {
      switch (key)
      {
      case 256: // Escape
        window.close();
        break;
      }
    }
    
    void onMouseDown(int button, int mods) override
    {
      if (button == GLFW_MOUSE_BUTTON_1)
        if (!dragging) // might be possible with two mice or something weird, not sure how glfw would handle that
          startDrag();
    }

    void onMouseUp(int button, int mods) override
    {
      if (button == GLFW_MOUSE_BUTTON_1)
        if (dragging)
          dragging = std::nullopt;
    }

  private:
    IGlWindow &window;
    struct Dragging { double x, y; };
    std::optional<Dragging> dragging;
    
    void drag(double xrel, double yrel) {
      // TODO: consider snapping to edges of screen work area:
      //   check if any edge is within some threshold distance to the nearest parallel edge of the screen work area;
      //   if multiple edges are within, take the closest per axis (may be snapping on horizontal and vertical edges);
      //   position the window so that the edge is exactly on the corresponding edge of the screen work area;
      //   this doesn't change any state except window position so shouldn't need to track anything.
      // NOTE: the snapping idea would need to take multiple monitors into account

      int sx, sy;
      window.getContentPosScreen(&sx, &sy);
      window.setContentPosScreen(sx + xrel - dragging->x, sy + yrel - dragging->y);
    }
    
    void startDrag() {
      dragging = Dragging{};
      window.getCursorPosContent(&dragging->x, &dragging->y);
    }
  };

//==============================================================================

  struct CallbackContext
  {
    GlWindowInputHandler &inputHandler;
    Mutexed<RenderThreadShared> &renderThreadShared;
    
    static CallbackContext *from(GLFWwindow *window) {
      return static_cast<CallbackContext*>(glfwGetWindowUserPointer(window));
    }
  };

//==============================================================================

  struct GlfwInputCallbacks
  {
    static void
    cursorPosition(GLFWwindow *window, double xpos, double ypos)
    {
      CallbackContext::from(window)->inputHandler.onCursorPosition(xpos, ypos);
    }

    static void
    mouseButton(GLFWwindow *window, int button, int action, int mods)
    {
      switch (auto &ih = CallbackContext::from(window)->inputHandler; action)
      {
        case GLFW_PRESS:
          ih.onMouseDown(button, mods);
          break;
        case GLFW_RELEASE:
          ih.onMouseUp(button, mods);
          break;
      }
    }

    static void
    key(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
      switch (auto &ih = CallbackContext::from(window)->inputHandler; action)
      {
      case GLFW_PRESS:
        ih.onKeyDown(key, scancode, mods);
        break;
      case GLFW_REPEAT:
        ih.onKeyRepeat(key, scancode, mods);
        break;
      case GLFW_RELEASE:
        ih.onKeyUp(key, scancode, mods);
        break;
      }
    }
  };

//==============================================================================

  struct GlfwRenderCallbacks
  {
    static void
    framebufferSize(GLFWwindow *window, int width, int height)
    {
      CallbackContext::from(window)->renderThreadShared.withLock(
          [=](RenderThreadShared &rts)
          { rts.frameSizeUpdate = {width, height}; });
    }

    static void
    windowRefresh(GLFWwindow *window)
    {
      CallbackContext::from(window)->renderThreadShared.withLockThenNotify(
          [=](RenderThreadShared &rts)
          { rts.state = RenderThreadState::shouldRender; });
    }
  };

//==============================================================================

  struct State
  {
    struct
    {
      Destroyer
          glfwInit,
          glfwCreateWindow;
    } destroyers;

    GLFWwindow *window{};
    std::thread renderThread;

    InputHandler inputHandler;
    Mutexed<RenderThreadShared> renderThreadShared{};
  };

//==============================================================================

  struct GlfwWindow : IGlWindow, State
  {
    void createGlfwWindow(CallbackContext *callbackContext)
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

      glfwSetWindowUserPointer( window, callbackContext );

      // glfw callbacks involving render thread
      glfwSetFramebufferSizeCallback( window, GlfwRenderCallbacks::framebufferSize );
      glfwSetWindowRefreshCallback( window, GlfwRenderCallbacks::windowRefresh );

      // glfw input callbacks (not render thread)
      glfwSetCursorPosCallback( window, GlfwInputCallbacks::cursorPosition );
      glfwSetKeyCallback( window, GlfwInputCallbacks::key );
      glfwSetMouseButtonCallback( window, GlfwInputCallbacks::mouseButton );
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
        futureGlRendererMaker
    )
      : State{.inputHandler{*this}}
      , callbackContext{.inputHandler = inputHandler, .renderThreadShared = renderThreadShared}
    {
      startGlfw();
      createGlfwWindow(&callbackContext);
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
    close()
    override
    {
      glfwSetWindowShouldClose(window, 1);
    }

    void
    enterEventLoop()
    override
    {
      for (startRenderThread(); !glfwWindowShouldClose(window); glfwWaitEvents());
    }
    
    void
    getCursorPosContent(double *x, double *y)
    override
    {
      glfwGetCursorPos( window, x, y );
    }

    void
    getContentPosScreen(int *x, int *y)
    override
    {
      glfwGetWindowPos( window, x, y );
    }

    void
    hide()
    override
    {
      glfwHideWindow( window );
    }
    
    void
    setContentPosScreen(int x, int y)
    override
    {
      glfwSetWindowPos( window, x, y );
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

      // get window frame size
      int left = 0, top = 0, right = 0, bottom = 0;
      glfwGetWindowFrameSize( window, &left, &top, &right, &bottom );

      const int availWidth = width - left - right;
      const int availHeight = height - top - bottom;
      
      int newWidth = 0, newHeight = 0;
      int newX = 0, newY = 0;

      if (contentWidth <= availWidth && contentHeight <= availHeight) {
        // set the window content size directly and then center the window within the work area
        newWidth = contentWidth;
        newHeight = contentHeight;
        newX = xpos + left + (availWidth / 2) - (contentWidth / 2);
        newY = ypos + top + (availHeight / 2) - (contentHeight / 2);
      }
      else {
        // content is larger than available space, so constrain while maintaining aspect ratio
        if (const float contentRatio = (float)contentWidth / contentHeight; contentRatio >= (float)availWidth / availHeight) {
          // content is wider than avail
          newWidth = availWidth;
          newHeight = (int)(newWidth / contentRatio);
          newX = xpos;
          newY = ypos + (availHeight - newHeight) / 2;
        }
        else {
          // content is taller than avail
          newHeight = availHeight;
          newWidth = (int)(newHeight * contentRatio);
          newX = xpos + (availWidth - newWidth) / 2;
          newY = ypos;
        }
      }

      glfwSetWindowSize( window, newWidth, newHeight );
      glfwSetWindowPos( window, newX, newY );
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
    
    private:
      CallbackContext callbackContext;
  };
} // namespace

std::unique_ptr<IGlWindow>
makeGlfwWindow(
    std::future<std::unique_ptr<IGlRendererMaker >> futureGlRendererMaker )
{
  return std::make_unique<GlfwWindow>( std::move( futureGlRendererMaker ));
}
