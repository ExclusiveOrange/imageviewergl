#include "makeShader.hpp"

GLuint
makeShader(
    const std::vector< char > &source,
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
  glDeleteShader( shader );

  throw std::runtime_error( errorLog.data() );
}
