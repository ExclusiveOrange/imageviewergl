#include "readFile.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <optional>

//==============================================================================

namespace
{
void glfwErrorCallback( int error, const char *description )
{
  std::cerr << "GLFW error: " << error << ": " << description << std::endl;
}

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

  GLint maxLength = 0;
  glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
  // The maxLength includes the NULL character
  std::vector< GLchar > errorLog( maxLength );
  glGetShaderInfoLog( shader, maxLength, &maxLength, &errorLog[0] );
  std::cout << "bad shader (" << filename << ") " << errorLog.data() << std::endl;

  glDeleteShader( shader );

  return {};
}

struct Destroyer
{
  const std::function< void( void ) > fnOnDestroy;
  ~Destroyer() { fnOnDestroy(); }
};
} // namespace

//==============================================================================

int main( int arc, char *argv[] )
{
  // TODO: check args: expect 1 filename
  // TODO: fork threads:
  //    A: try to load filename as image then join B
  //    B: initialize glfw and create a window then wait for A
  // TODO: if error in A then when A joins B, display message and quit
  // TODO: else if no error (image loaded into CPU memory) then:
  //    * load image into GPU memory (GL texture)
  //    * change window size to fit image
  //    * draw image initially, then as needed

  if( !glfwInit())
  {
    std::cerr << "glfwInit() failed" << std::endl;
    return 1;
  }
  Destroyer _glfwInit{ glfwTerminate };

  //------------------------------------------------------------------------------

  glfwSetErrorCallback( glfwErrorCallback );

  //------------------------------------------------------------------------------

  for( auto[hint, value] : (int[][2]){
      { GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE },
      { GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE },
      { GLFW_CONTEXT_VERSION_MAJOR, 4 },
      { GLFW_CONTEXT_VERSION_MINOR, 1 }} )
    glfwWindowHint( hint, value );

  GLFWwindow *window = glfwCreateWindow( 640, 480, "My Title", nullptr, nullptr );
  if( !window )
  {
    std::cerr << "glfwCreateWindow(..) failed" << std::endl;
    return 1;
  }
  Destroyer _glfwCreateWindow{ [window] { glfwDestroyWindow( window ); }};

  //------------------------------------------------------------------------------

  glfwMakeContextCurrent( window );

  //------------------------------------------------------------------------------

  if( GLEW_OK != glewInit())
  {
    std::cerr << "glewInit() failed" << std::endl;
    return 1;
  }

  //------------------------------------------------------------------------------

  GLuint texture{};
  glGenTextures( 1, &texture );
  glBindTexture( GL_TEXTURE_2D, texture );
  {
    int width, height, nChannels;
    stbi_uc *image = stbi_load( "../texture.jpg", &width, &height, &nChannels, 0 );
    // TODO: error check result of stbi_load
    assert( nChannels > 0 && nChannels <= 4 );

    GLenum formatByNumChannels[4]{
        GL_RED,
        GL_RG,
        GL_RGB,
        GL_RGBA
    };
    GLenum format = formatByNumChannels[nChannels - 1];

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, image );

    if( nChannels < 3 )
    {
      GLint swizzleMask[2][4]{
          { GL_RED, GL_RED, GL_RED, GL_ONE },
          { GL_RED, GL_RED, GL_RED, GL_GREEN }};

      glTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask[nChannels - 1] );
    }

    stbi_image_free( image );
  }
  Destroyer _texture{ [&texture]{ glDeleteTextures(1, &texture); }};

  //------------------------------------------------------------------------------

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

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
  Destroyer _shaderProgram{ [shaderProgram]{ glDeleteProgram( shaderProgram ); }};
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
  Destroyer _emptyVertexArray{ [emptyVertexArray]{ glDeleteVertexArrays(1, &emptyVertexArray); }};
  glBindVertexArray( emptyVertexArray );

  //------------------------------------------------------------------------------

  // Event loop
  while( !glfwWindowShouldClose( window ))
  {
    // TODO: wait for signal to draw a frame; maybe wait on a condition variable or something

    // Clear the screen to black
//            glClearColor( 0.5f, 0.0f, 0.0f, 1.0f );
//            glClear( GL_COLOR_BUFFER_BIT );

    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

    glfwSwapBuffers( window );
    glfwPollEvents();
  }
}