#include "readFile.hpp"
#include "Destroyer.hpp"
#include "GlfwWindow.hpp"

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
  throw std::runtime_error( errorLog.data() );

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

void
prepareMakeGlRenderer(
    std::string imageFilename,
    std::promise< makeGlRenderer_t > promiseMakeGlRenderer )
{
  struct Image
  {
    int width{}, height{}, nChannels{};
    stbi_uc *pixels{};
  };

  //------------------------------------------------------------------------------

  std::optional< Image > maybeImage;
  std::thread imageLoadingThread{
      [&imageFilename, &maybeImage]
      {
        Image image;
        image.pixels = stbi_load( imageFilename.c_str(), &image.width, &image.height, &image.nChannels, 0 );
        if( !image.pixels )
          std::cerr << "failed to load image " << imageFilename << "\nbecause: " << stbi_failure_reason() << std::endl;
        else
          maybeImage = image;
      }};

  // TODO: define closure lambda:
  //   (void) -> std::unique_ptr< IGlRenderer >
  // NOTE: this
  // TODO: set promise value
}

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

  std::promise< makeGlRenderer_t > promiseMakeGlRenderer;
  std::future< makeGlRenderer_t > futureMakeGlRenderer = promiseMakeGlRenderer.get_future();

  std::thread preparingMakeGlRenderer{
      prepareMakeGlRenderer,
      imageFilename,
      std::move( promiseMakeGlRenderer ) };

  //------------------------------------------------------------------------------

  std::unique_ptr< IGlWindow > window = makeGlfwWindow( std::move( futureMakeGlRenderer ));

  //------------------------------------------------------------------------------

  std::vector< char > vertShaderSource = readFile("../shaders/texture.vert");
  GLuint vertShader{};
  if( std::optional< GLuint > maybeVertShader = makeShader( vertShaderSource, GL_VERTEX_SHADER ))
    vertShader = *maybeVertShader;
  else
    return 1;
  Destroyer _vertShader{ [vertShader] { glDeleteShader( vertShader ); }};

  //------------------------------------------------------------------------------

  std::vector< char > fragShaderSource = readFile("../shaders/texture.frag");
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

  imageLoadingThread.join();

  if( !maybeImage )
    return 1;

  Image image = *maybeImage;
  maybeImage.reset();

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
}