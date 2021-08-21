#include "readFile.hpp"

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

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
loadShader(const char *filename, GLenum shaderType)
{
  GLuint shader = glCreateShader( shaderType );
  const std::vector< char > source = readFile( filename );
  const GLchar *pVertShaderSource[] = { source.data() }; // need GLchar**
  glShaderSource( shader, 1, pVertShaderSource, NULL );

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

  std::cout << "bad vert shader: " << errorLog.data() << std::endl;

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
        struct RGBA { GLubyte r, g, b, a; };
        const GLsizei texWidth = 128, texHeight = 128;

        RGBA *texRaw = new RGBA[texWidth * texHeight];

        for( GLsizei y = 0; y < texHeight; ++y )
          for( GLsizei x = 0; x < texWidth; ++x )
            texRaw[y * texWidth + x] = { (GLubyte)x, (GLubyte)y, 0, 255 };

        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texRaw );

        delete[] texRaw;
      }

      if( std::optional< GLuint > maybeVertShader = loadShader("../shaders/texture.vert", GL_VERTEX_SHADER))
      {
        GLuint vertShader = *maybeVertShader;

        if( std::optional< GLuint > maybeFragShader = loadShader("../shaders/texture.frag", GL_FRAGMENT_SHADER))
        {
          GLuint fragShader = *maybeFragShader;

          // Event loop
          while( !glfwWindowShouldClose( window ))
          {
            // Clear the screen to black
            glClearColor( 0.5f, 0.0f, 0.0f, 1.0f );
            glClear( GL_COLOR_BUFFER_BIT );
            glfwSwapBuffers( window );
            glfwPollEvents();
          }

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