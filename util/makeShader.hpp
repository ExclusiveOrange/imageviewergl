#pragma once

#define GL_SILENCE_DEPRECATION // MacOS has deprecated OpenGL - it still works up to 4.1 for now
#include <gl/glew.h>

#include <vector>

GLuint
makeShader(
    const std::vector< char > &source,
    GLenum shaderType )
noexcept( false );
