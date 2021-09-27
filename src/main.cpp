#include "Destroyer.hpp"
#include "GlfwWindow.hpp"
#include "makeGlRendererMaker.hpp"
#include "readImageDimensions.hpp"

#include <filesystem>
#include <future>
#include <iostream>

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

  // In a separate thread calls makeGlRendererMaker through std::async;
  // at the same time, passes a std::future to makeGlfwWindow(..),
  // which will be fulfilled when the image has been loaded in makeGlRendererMaker.
  // This allows makeGlfwWindow to create the window and initialize GLFW and OpenGL
  // simultaneously with the image being loaded from the filesystem, to hopefully
  // reduce the total time it takes before the user sees the image on screen.

  std::unique_ptr< IGlWindow > window = makeGlfwWindow(
      std::async( std::launch::async, makeGlRendererMaker, imageFilename ));

  //------------------------------------------------------------------------------

  window->setTitle( imageFilename.c_str());

  const ImageDimensions imageDimensions = readImageDimensions( imageFilename.c_str() );

  window->setContentAspectRatio( imageDimensions.width, imageDimensions.height );
  window->setContentSize( imageDimensions.width, imageDimensions.height );

  window->show();

  //------------------------------------------------------------------------------

  window->enterEventLoop();
}