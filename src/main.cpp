#include "Destroyer.hpp"
#include "GlfwWindow.hpp"
#include "GlWindowInputHandler.hpp"
#include "makeGlRendererMaker.hpp"
#include "readImageDimensions.hpp"

#include <codecvt>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <locale>

//==============================================================================

// thanks https://stackoverflow.com/a/13556072
// this solves a problem that CMake + CLion cannot: different link options for Windows depending on Debug or Release
#ifdef _MSC_VER
#include <windows.h>
#   ifdef NDEBUG
#     pragma message("Windows Release Mode")
#     pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#   else
#     pragma message("Windows Debug Mode")
#     pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#   endif
#endif

//==============================================================================

/* TODO: Error dialog.
Make error reporting create a dialog (is this possible with glfw?) when there is an error.
Currently errors are only visible if you launch the program from a command line terminal,
otherwise the program appears to quit without saying anything.
Currently cases where the program might fail are:
  * missing shader files (I'd like to bake them into the executable but I don't know a simple cross-platform way to do that, yet
  * corrupt or unsupported image file / format
  * out of memory (I guess this is possible but very unlikely)
*/

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

int Main( int argc, char *argv[] )
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
  Destroyer revertWorkingDirectory{ [=] { std::filesystem::current_path( initialWorkingDirectory ); }};

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

  std::unique_ptr<IGlWindow> window = makeGlfwWindow(
      std::async( std::launch::async, makeGlRendererMaker, imageFilename ));

  //------------------------------------------------------------------------------

  window->setTitle( imageFilename );

  const ImageDimensions imageDimensions = readImageDimensions( imageFilename.c_str());

  window->setContentAspectRatio( imageDimensions.width, imageDimensions.height );
  window->setContentSize( imageDimensions.width, imageDimensions.height );

  window->show();

  //------------------------------------------------------------------------------

  // useless for now but could be used for something later
  struct InputHandler : GlWindowInputHandler
  {
    void onCursorPosition( double xPos, double yPos ) override
    {
      // std::cout << "onCursorPosition (" << xPos << ", " << yPos << ")\n";
    }

    void onScroll( double xAmount, double yAmount ) override
    {
      // std::cout << "onScroll (" << xAmount << ", " << yAmount << ")\n";
    }
  };

  window->enterEventLoop( std::make_unique<InputHandler>());

  return 0;
}

//==============================================================================

#ifdef _MSC_VER

int main( int /*argc*/, char */*argv*/[] )
{
  setlocale( LC_ALL, ".UTF8" );

  int argc;
  char **args;

  {
    LPWSTR *wArgs = CommandLineToArgvW( GetCommandLineW(), &argc );

    if( argc < 1 )
      return -1;

    // convert wchar_t args to utf8 args
    args = new char *[argc];
    {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      for( int i = 0; i < argc; ++i )
        args[i] = _strdup( converter.to_bytes( wArgs[i] ).c_str());
    }

    LocalFree( wArgs );
  }

  // call the real Main
  int mainRet = Main( argc, args );

  // free utf8 args
  for( int i = 0; i < argc; ++i )
    free( args[i] );
  delete args;

  return mainRet;
}

#else

int main(int argc, char *argv[])
{
  setlocale( LC_ALL, ".UTF8" );

  // call the real Main
  return Main(argc, argv);
}

#endif

