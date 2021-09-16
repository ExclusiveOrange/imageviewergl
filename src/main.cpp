#include "Destroyer.hpp"
#include "GlRenderer_ImageRenderer.hpp"
#include "GlfwWindow.hpp"
#include "makeUniqueFunctor.hpp"
#include "RawImage_StbImage.hpp"

#include <filesystem>
#include <iostream>
#include <thread>

//==============================================================================

namespace
{
//void centerGlfwWindow( GLFWwindow *window, GLFWmonitor *monitor )
//{
//  struct { int x, y, w, h; } monitorWorkArea;
//  glfwGetMonitorWorkarea( monitor, &monitorWorkArea.x, &monitorWorkArea.y, &monitorWorkArea.w, &monitorWorkArea.h );
//  struct { int x, y; } monitorWorkAreaCenter{
//      monitorWorkArea.w / 2 + monitorWorkArea.x,
//      monitorWorkArea.h / 2 + monitorWorkArea.y };
//
//  struct { int w, h; } windowSize;
//  glfwGetWindowSize( window, &windowSize.w, &windowSize.h );
//
//  struct { int l, t, r, b; } frameSize;
//  glfwGetWindowFrameSize( window, &frameSize.l, &frameSize.t, &frameSize.r, &frameSize.b );
//
//  struct { int w, h; } fullWindowSize{
//      windowSize.w + frameSize.l + frameSize.r,
//      windowSize.h + frameSize.t + frameSize.b };
//
//  struct { int x, y; } windowContentPos{
//      monitorWorkAreaCenter.x - fullWindowSize.w / 2 + frameSize.l,
//      monitorWorkAreaCenter.y - fullWindowSize.h / 2 + frameSize.t };
//
//  glfwSetWindowPos( window, windowContentPos.x, windowContentPos.y );
//}
} // namespace

//==============================================================================

int main( int argc, char *argv[] )
{
  // TODO: add "dear imgui", for eventual messages or image information or application settings

  if( argc != 2 )
  {
    std::cout << "usage: " << argv[0] << " path/to/someImage.jpg" << std::endl;
    return 1;
  }

  //------------------------------------------------------------------------------

  // remember initial working directory (might be not exe directory)
  const std::filesystem::path initialWorkingDirectory = std::filesystem::current_path();
  Destroyer revertWorkingDirectory{ [&] { std::filesystem::current_path( initialWorkingDirectory ); }};

  {
    // set working directory to this exe (so dll's can be found, at least on Windows)
    std::filesystem::path fullExePath = std::filesystem::current_path() / argv[0];
    fullExePath.remove_filename();
    std::filesystem::current_path( fullExePath );
  }

  //------------------------------------------------------------------------------

  std::string imageFilename = (initialWorkingDirectory / argv[1]).string();

  //------------------------------------------------------------------------------

  // The following with a promise, a future, a thread, and some confusing stuff:
  //   We want to load the image from storage in a separate thread so that it doesn't
  //   delay creation of the window and initialization of OpenGL.
  //   We also want to allow creation of GlRenderer_ImageRenderer, but only after the
  //   image has been loaded, and only if there were no errors.
  //   We don't want to create the GlRenderer_ImageRenderer here because it will need
  //   the OpenGL context to exist, and it doesn't exist yet, and we don't control it.
  //   So instead we wait until our image is loaded, and then we provide a function
  //   which can create a GlRenderer_ImageRenderer.
  //   We give this function to the IGlWindow through a promise/future mechanism in which
  //   we give the future to the IGlWindow through its constructor.
  //   The IGlWindow can periodically check whether the future is ready, at which point it
  //   can call our function which creates a GlRenderer_ImageRenderer.

  std::thread preparingMakeGlRenderer;
  std::unique_ptr< IGlWindow > window;

  {
    std::promise< makeGlRenderer_t > promiseMakeGlRenderer;
    std::future< makeGlRenderer_t > futureMakeGlRenderer = promiseMakeGlRenderer.get_future();

    preparingMakeGlRenderer = std::thread{
        [imageFilename, promise = std::move( promiseMakeGlRenderer )]() mutable
        {
          try
          {
            std::unique_ptr< IRawImage > rawImage =
                loadRawImage_StbImage( imageFilename.c_str());

            promise.set_value(
                makeUniqueFunctor< std::unique_ptr< IGlRenderer >>(
                    [rawImage = std::move( rawImage )]() mutable
                    {
                      return makeGlRenderer_ImageRenderer( std::move( rawImage ));
                    } ));
          }
          catch( ... )
          {
            promise.set_exception( std::current_exception() );
          }
        }};

    window = makeGlfwWindow( std::move( futureMakeGlRenderer ));
  }

  //------------------------------------------------------------------------------

  window->setTitle( imageFilename.c_str());

  //------------------------------------------------------------------------------

  // TODO: move window rescaling based on image size
  //   somehow into GlRenderer_ImageRenderer
//
//  {
//    auto[xscale, yscale] = window->getContentScale();
//    const int scaledWidth = int( xscale * float( image.width ));
//    const int scaledHeight = int( yscale * float( image.height ));
//    window->setContentSize( scaledWidth, scaledHeight );
//  }
//  window->setContentAspectRatio( image.width, image.height );
  // TODO: recreate centering mechanism using IGlWindow methods
  //centerGlfwWindow( window, glfwGetPrimaryMonitor());

  //------------------------------------------------------------------------------

  window->show();

  //------------------------------------------------------------------------------

  window->enterEventLoop();
  preparingMakeGlRenderer.join();
}