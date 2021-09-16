#include "Destroyer.hpp"
#include "ErrorString.hpp"
#include "GlRenderer_ImageRenderer.hpp"
#include "makeShader.hpp"
#include "readFile.hpp"

namespace
{
constexpr const char *vertShaderFilename = "../shaders/texture.vert";
constexpr const char *fragShaderFilename = "../shaders/texture.frag";

struct GlRenderer : public IGlRenderer
{
  GLuint emptyVertexArray{};
  Destroyer _emptyVertexArray;

  GLuint texture{};
  Destroyer _texture;

  GLuint vertShader{};
  Destroyer _vertShader;

  GLuint fragShader{};
  Destroyer _fragShader;

  GLuint shaderProgram{};
  Destroyer _shaderProgram;

  void makeEmptyVertexArray()
  {
    // get error 1282 from glDrawArrays when I don't use any vertex array objects...
    // shouldn't need any because the vertices are generated in the vert shader, but maybe I need something here...
    glGenVertexArrays( 1, &emptyVertexArray );
    _emptyVertexArray = Destroyer{ [this] { glDeleteVertexArrays( 1, &this->emptyVertexArray ); }};
  }

  void makeShaderProgram()
  noexcept( false )
  {
    vertShader = makeShader( readFile( vertShaderFilename ), GL_VERTEX_SHADER );
    _vertShader = Destroyer{ [this] { glDeleteShader( this->vertShader ); }};

    fragShader = makeShader( readFile( fragShaderFilename ), GL_FRAGMENT_SHADER );
    _fragShader = Destroyer{ [this] { glDeleteShader( this->fragShader ); }};

    shaderProgram = glCreateProgram();
    _shaderProgram = Destroyer{ [this] { glDeleteProgram( this->shaderProgram ); }};
    glAttachShader( shaderProgram, vertShader );
    glAttachShader( shaderProgram, fragShader );
    glLinkProgram( shaderProgram );
    if( GLint linkStatus; glGetProgramiv( shaderProgram, GL_LINK_STATUS, &linkStatus ), linkStatus == GL_FALSE )
      throw ErrorString( __FUNCTION__, " error: glLinkProgram(..) failed!" );
  }

  void makeTextureFromImage( std::unique_ptr< IRawImage > rawImage )
  {
    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    {
      const GLenum formatByNumChannels[4]{ GL_RED, GL_RG, GL_RGB, GL_RGBA };
      GLenum format = formatByNumChannels[rawImage->getNChannels() - 1];

      // odd-width RGB source images are misaligned byte-wise without these next four lines
      // thanks: https://stackoverflow.com/a/7381121
      glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
      glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
      glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
      glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );

      glTexImage2D(
          GL_TEXTURE_2D, 0,
          GL_RGBA, rawImage->getWidth(), rawImage->getHeight(),
          0,
          format, GL_UNSIGNED_BYTE, rawImage->getPixels());

      if( rawImage->getNChannels() < 3 )
      {
        GLint swizzleMask[2][4]{
            { GL_RED, GL_RED, GL_RED, GL_ONE },
            { GL_RED, GL_RED, GL_RED, GL_GREEN }};

        glTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask[rawImage->getNChannels() - 1] );
      }

      rawImage.reset();

      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    }
    _texture = Destroyer{ [this] { glDeleteTextures( 1, &this->texture ); }};
  }

  GlRenderer( std::unique_ptr< IRawImage > rawImage )
  noexcept( false )
  {
    makeTextureFromImage( std::move( rawImage ));
    makeShaderProgram();
    makeEmptyVertexArray();
  }

  virtual ~GlRenderer() override = default;

  virtual void render() override
  {
    glUseProgram( shaderProgram );
    glBindTexture( GL_TEXTURE_2D, texture );
    glBindVertexArray( emptyVertexArray );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
  }
};
} // namespace

std::unique_ptr< IGlRenderer >
makeGlRenderer_ImageRenderer( std::unique_ptr< IRawImage > rawImage )
{
  return std::make_unique< GlRenderer >( std::move( rawImage ));
}
