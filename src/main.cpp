#include "readFile.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <cassert>
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

  return {};
}
} // namespace

//==============================================================================

int main()
{
  if( glfwInit())
  {
    glfwSetErrorCallback( glfwErrorCallback );

    struct { int hint, value; } windowHints[]{
        { GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE },
        { GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE },
//        { GLFW_RESIZABLE, GL_FALSE },
        { GLFW_CONTEXT_VERSION_MAJOR, 4 },
        { GLFW_CONTEXT_VERSION_MINOR, 1 }
    };

    for( auto windowHint : windowHints )
      glfwWindowHint( windowHint.hint, windowHint.value );

    if( GLFWwindow *window = glfwCreateWindow( 640, 480, "My Title", NULL, NULL ))
    {
      glfwMakeContextCurrent( window );

      // create texture for testing
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

          glTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask[nChannels-1] );
        }

        stbi_image_free( image );
      }

      if( std::optional< GLuint > maybeVertShader = loadShader( "../shaders/texture.vert", GL_VERTEX_SHADER ))
      {
        GLuint vertShader = *maybeVertShader;

        if( std::optional< GLuint > maybeFragShader = loadShader( "../shaders/texture.frag", GL_FRAGMENT_SHADER ))
        {
          GLuint fragShader = *maybeFragShader;

          GLuint shaderProgram = glCreateProgram();
          glAttachShader( shaderProgram, vertShader );
          glAttachShader( shaderProgram, fragShader );
          glLinkProgram( shaderProgram );
          GLint linkStatus;
          glGetProgramiv( shaderProgram, GL_LINK_STATUS, &linkStatus );
          if( linkStatus == GL_FALSE )
          {
            std::cerr << "shader program link failed!\n";
            // TODO: refactor to handle this error; probably should do the shaders and link the program all in one, separate function
          }

          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
          glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

          glDisable( GL_CULL_FACE );
          glUseProgram( shaderProgram );

          // get error 1282 from glDrawArrays when I don't use any vertex array objects...
          // shouldn't need any because the vertices are generated in the vert shader, but maybe I need something here...
          GLuint vao = 0;
          glGenVertexArrays( 1, &vao );
          glBindVertexArray( vao );

          // Event loop
          while( !glfwWindowShouldClose( window ))
          {
            // Clear the screen to black
//            glClearColor( 0.5f, 0.0f, 0.0f, 1.0f );
//            glClear( GL_COLOR_BUFFER_BIT );

            glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

            glfwSwapBuffers( window );
            glfwPollEvents();
          }

          glDeleteProgram( shaderProgram );

          glDeleteShader( fragShader );
        }

        glDeleteShader( vertShader );
      }

      // destroy texture
      glDeleteTextures( 1, &texture );
    }
    else
      std::cerr << "glfwCreateWindow(..) failed" << std::endl;

    glfwTerminate();
  }
  else
  {
    std::cerr << "glfwInit() failed" << std::endl;
    return 1;
  }
}