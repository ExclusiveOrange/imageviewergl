
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <iostream>

//==============================================================================

namespace
{
void glfwErrorCallback( int error, const char *description )
{
  std::cerr << "GLFW error: " << error << ": " << description << std::endl;
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
      // Event loop
      while( !glfwWindowShouldClose( window ))
      {
        // Clear the screen to black
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
        glfwSwapBuffers( window );
        glfwPollEvents();
      }

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