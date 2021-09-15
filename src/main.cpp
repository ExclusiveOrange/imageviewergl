#include "Destroyer.hpp"
#include "GlRenderer_ImageRenderer.hpp"
#include "GlfwWindow.hpp"
#include "makeUniqueFunctor.hpp"
#include "RawImage_StbImage.hpp"
#include "readFile.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>

//==============================================================================

namespace
{
std::optional< GLuint >
makeShader(
    const std::vector< char > source,
    GLenum shaderType )
{
  const GLchar *pShaderSource[] = { source.data() }; // need GLchar**
  const GLint shaderSourceLength[] = { (GLint)source.size() };
  GLuint shader = glCreateShader( shaderType );
  glShaderSource( shader, 1, pShaderSource, shaderSourceLength );

  GLint isCompiled = 0;
  glCompileShader( shader );
  glGetShaderiv( shader, GL_COMPILE_STATUS, &isCompiled );
  if( isCompiled == GL_TRUE )
    return shader; // success

  // error happened: get details and return nullopt
  GLint logLength = 0;
  glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
  // The logLength includes the NULL character
  std::vector< GLchar > errorLog( logLength );
  glGetShaderInfoLog( shader, logLength, &logLength, &errorLog[0] );
  throw std::runtime_error( errorLog.data());

  glDeleteShader( shader );

  return std::nullopt;
}

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
                makeRawImage_StbImage( imageFilename.c_str());

            promise.set_value(
                makeUniqueFunctor< std::unique_ptr< IGlRenderer >>(
                    [rawImage = std::move( rawImage )]() mutable
                    {
                      return makeGlRenderer_ImageRenderer( std::move( rawImage ));
                    } ));
          }
          catch( const std::exception &e )
          {
            promise.set_exception( std::make_exception_ptr( e ));
          }
        }};

    window = makeGlfwWindow( std::move( futureMakeGlRenderer ));
  }

  //------------------------------------------------------------------------------

  std::vector< char > vertShaderSource = readFile( "../shaders/texture.vert" );
  GLuint vertShader{};
  if( std::optional< GLuint > maybeVertShader = makeShader( vertShaderSource, GL_VERTEX_SHADER ))
    vertShader = *maybeVertShader;
  else
    return 1;
  Destroyer _vertShader{ [vertShader] { glDeleteShader( vertShader ); }};

  //------------------------------------------------------------------------------

  std::vector< char > fragShaderSource = readFile( "../shaders/texture.frag" );
  GLuint fragShader{};
  if( std::optional< GLuint > maybeFragShader = makeShader( fragShaderSource, GL_FRAGMENT_SHADER ))
    fragShader = *maybeFragShader;
  else
    return 1;
  Destroyer _fragShader{ [fragShader] { glDeleteShader( fragShader ); }};

  //------------------------------------------------------------------------------

  GLuint shaderProgram = glCreateProgram();
  Destroyer _shaderProgram{ [shaderProgram] { glDeleteProgram( shaderProgram ); }};
  glAttachShader( shaderProgram, vertShader );
  glAttachShader( shaderProgram, fragShader );
  glLinkProgram( shaderProgram );
  if( GLint linkStatus; glGetProgramiv( shaderProgram, GL_LINK_STATUS, &linkStatus ), linkStatus == GL_FALSE )
  {
    std::cerr << "shader program link failed!\n";
    return 1;
  }
  glUseProgram( shaderProgram );

  //------------------------------------------------------------------------------

  // get error 1282 from glDrawArrays when I don't use any vertex array objects...
  // shouldn't need any because the vertices are generated in the vert shader, but maybe I need something here...
  GLuint emptyVertexArray = 0;
  glGenVertexArrays( 1, &emptyVertexArray );
  Destroyer _emptyVertexArray{ [emptyVertexArray] { glDeleteVertexArrays( 1, &emptyVertexArray ); }};
  glBindVertexArray( emptyVertexArray );

  //------------------------------------------------------------------------------

  GLuint texture{};
  glGenTextures( 1, &texture );
  glBindTexture( GL_TEXTURE_2D, texture );
  {
    const GLenum formatByNumChannels[4]{ GL_RED, GL_RG, GL_RGB, GL_RGBA };
    GLenum format = formatByNumChannels[image.nChannels - 1];

    // odd-width RGB source images are misaligned byte-wise without these next four lines
    // thanks: https://stackoverflow.com/a/7381121
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.pixels );

    if( image.nChannels < 3 )
    {
      GLint swizzleMask[2][4]{
          { GL_RED, GL_RED, GL_RED, GL_ONE },
          { GL_RED, GL_RED, GL_RED, GL_GREEN }};

      glTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask[image.nChannels - 1] );
    }

    stbi_image_free( image.pixels );
    image.pixels = {};
  }
  Destroyer _texture{ [&texture] { glDeleteTextures( 1, &texture ); }};

  //------------------------------------------------------------------------------

  window->setTitle( imageFilename.c_str());

  //------------------------------------------------------------------------------

  {
    auto[xscale, yscale] = window->getContentScale();
    const int scaledWidth = int( xscale * float( image.width ));
    const int scaledHeight = int( yscale * float( image.height ));
    window->setContentSize( scaledWidth, scaledHeight );
  }
  window->setContentAspectRatio( image.width, image.height );
  // TODO: recreate centering mechanism using IGlWindow methods
  //centerGlfwWindow( window, glfwGetPrimaryMonitor());

  //------------------------------------------------------------------------------

  window->show();

  //------------------------------------------------------------------------------

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  //------------------------------------------------------------------------------

  window->setRenderFunction(
      []
      {
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
      } );

  //------------------------------------------------------------------------------

  window->enterEventLoop();
  preparingMakeGlRenderer.join();
}