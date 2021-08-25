#include "readFile.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <cassert>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

//==============================================================================

namespace
{
struct Destroyer
{
  const std::function< void( void ) > fnOnDestroy;

  ~Destroyer() { fnOnDestroy(); }
};

//------------------------------------------------------------------------------

enum class RenderThreadState { shouldWait, shouldRender, shouldQuit };
struct FrameSize { int width, height; };

struct
{
  RenderThreadState state = RenderThreadState::shouldRender;
  std::mutex mutex;
  std::condition_variable condVar;
  std::optional< FrameSize > frameSizeUpdate;
} renderThreadShared;

//------------------------------------------------------------------------------

void _glfwErrorCallback( int error, const char *description )
{
  std::cerr << "GLFW error: " << error << ": " << description << std::endl;
}

void _glfwFrameSizeCallback( GLFWwindow *window, int width, int height )
{
  std::unique_lock lk( renderThreadShared.mutex );
  renderThreadShared.frameSizeUpdate = { width, height };
//  renderThreadShared.state = RenderThreadState::shouldRender;
//  renderThreadShared.condVar.notify_one();
}

void _glfwWindowRefreshCallback( GLFWwindow *window )
{
  std::unique_lock lk( renderThreadShared.mutex );
  renderThreadShared.state = RenderThreadState::shouldRender;
  renderThreadShared.condVar.notify_one();
}

//------------------------------------------------------------------------------

std::optional< GLuint >
loadShader( const char *filename, GLenum shaderType )
{
  GLuint shader = glCreateShader( shaderType );
  const std::vector< char > source = readFile( filename );
  const GLchar *pShaderSource[] = { source.data() }; // need GLchar**
  const GLint shaderSourceLength[] = { (GLint)source.size() };
  glShaderSource( shader, 1, pShaderSource, shaderSourceLength );

  GLint isCompiled = 0;
  glCompileShader( shader );
  glGetShaderiv( shader, GL_COMPILE_STATUS, &isCompiled );
  if( isCompiled == GL_TRUE )
    return shader;

  GLint logLength = 0;
  glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
  // The logLength includes the NULL character
  std::vector< GLchar > errorLog( logLength );
  glGetShaderInfoLog( shader, logLength, &logLength, &errorLog[0] );
  std::cout << "bad shader (" << filename << ") " << errorLog.data() << std::endl;

  glDeleteShader( shader );

  return {};
}

void centerGlfwWindow( GLFWwindow *window, GLFWmonitor *monitor )
{
  struct { int x, y, w, h; } monitorWorkArea;
  glfwGetMonitorWorkarea( monitor, &monitorWorkArea.x, &monitorWorkArea.y, &monitorWorkArea.w, &monitorWorkArea.h );
  struct { int x, y; } monitorWorkAreaCenter{
      monitorWorkArea.w / 2 + monitorWorkArea.x,
      monitorWorkArea.h / 2 + monitorWorkArea.y };

  struct { int w, h; } windowSize;
  glfwGetWindowSize( window, &windowSize.w, &windowSize.h );

  struct { int l, t, r, b; } frameSize;
  glfwGetWindowFrameSize( window, &frameSize.l, &frameSize.t, &frameSize.r, &frameSize.b );

  struct { int w, h; } fullWindowSize{
      windowSize.w + frameSize.l + frameSize.r,
      windowSize.h + frameSize.t + frameSize.b };

  struct { int x, y; } windowContentPos{
      monitorWorkAreaCenter.x - fullWindowSize.w / 2 + frameSize.l,
      monitorWorkAreaCenter.y - fullWindowSize.h / 2 + frameSize.t };

  glfwSetWindowPos( window, windowContentPos.x, windowContentPos.y );
}
} // namespace

//==============================================================================

int main( int argc, char *argv[] )
{
  // TODO: refactor into separate functions; maybe with a State struct or something to share data...
  //       maybe put all this in an App class... something to organize it better

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

  //------------------------------------------------------------------------------

  glfwSetErrorCallback( _glfwErrorCallback );

  //------------------------------------------------------------------------------

  if( !glfwInit())
  {
    std::cerr << "glfwInit() failed" << std::endl;
    return 1;
  }
  Destroyer _glfwInit{ glfwTerminate };

  //------------------------------------------------------------------------------

  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
  glfwWindowHint( GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE );
  glfwWindowHint( GLFW_DECORATED, GLFW_TRUE );

  GLFWwindow *window = glfwCreateWindow( 640, 480, "My Title", nullptr, nullptr );
  if( !window )
  {
    std::cerr << "glfwCreateWindow(..) failed" << std::endl;
    return 1;
  }
  Destroyer _glfwCreateWindow{ [window] { glfwDestroyWindow( window ); }};

  //------------------------------------------------------------------------------

  // DELETE: get current monitor work area and print it
  {
    int xpos, ypos, width, height;
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorWorkarea( monitor, &xpos, &ypos, &width, &height );
    std::cout << "monitor work area: {" << xpos << ", " << ypos << ", " << width << ", " << height << std::endl;
  }

  //------------------------------------------------------------------------------

  glfwSetFramebufferSizeCallback( window, _glfwFrameSizeCallback );
  glfwSetWindowRefreshCallback( window, _glfwWindowRefreshCallback );
  glfwMakeContextCurrent( window );

  //------------------------------------------------------------------------------

  if( GLEW_OK != glewInit())
  {
    std::cerr << "glewInit() failed" << std::endl;
    return 1;
  }

  //------------------------------------------------------------------------------

  GLuint vertShader{};
  if( std::optional< GLuint > maybeVertShader = loadShader( "../shaders/texture.vert", GL_VERTEX_SHADER ))
    vertShader = *maybeVertShader;
  else
    return 1;
  Destroyer _vertShader{ [vertShader] { glDeleteShader( vertShader ); }};

  //------------------------------------------------------------------------------

  GLuint fragShader{};
  if( std::optional< GLuint > maybeFragShader = loadShader( "../shaders/texture.frag", GL_FRAGMENT_SHADER ))
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

  glfwSetWindowTitle( window, imageFilename.c_str());

  //------------------------------------------------------------------------------

  {
    float xscale = 1.f, yscale = 1.f;
    glfwGetWindowContentScale( window, &xscale, &yscale );
    const int scaledWidth = int( xscale * float( image.width ));
    const int scaledHeight = int( yscale * float( image.height ));
    glfwSetWindowSize( window, scaledWidth, scaledHeight );
  }
  glfwSetWindowAspectRatio( window, image.width, image.height );
  centerGlfwWindow( window, glfwGetPrimaryMonitor());

  //------------------------------------------------------------------------------

  glfwShowWindow( window );

  //------------------------------------------------------------------------------

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  //------------------------------------------------------------------------------

  glfwMakeContextCurrent( nullptr );

  std::thread renderThread{
      [window]
      {
        glfwMakeContextCurrent( window );
        glfwSwapInterval( 0 );

        for( ;; )
        {
          std::unique_lock lk( renderThreadShared.mutex );
          renderThreadShared.condVar
                            .wait( lk, [&] { return renderThreadShared.state != RenderThreadState::shouldWait; } );

          if( renderThreadShared.state == RenderThreadState::shouldQuit )
            break;

          if( std::optional< FrameSize >
              maybeFrameSize = std::exchange( renderThreadShared.frameSizeUpdate, std::nullopt ))
            glViewport( 0, 0, maybeFrameSize->width, maybeFrameSize->height );

          glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
          glfwSwapBuffers( window );

          renderThreadShared.state = RenderThreadState::shouldWait;
        }
      }};

  //------------------------------------------------------------------------------

  // Event loop
  while( !glfwWindowShouldClose( window ))
    glfwWaitEvents();

  {
    std::unique_lock lk( renderThreadShared.mutex );
    renderThreadShared.state = RenderThreadState::shouldQuit;
    renderThreadShared.condVar.notify_one();
  }

  renderThread.join();

  return 0;
}