#include "ErrorString.hpp"
#include "RawImage_StbImage.hpp"

#include <stb_image.h>

namespace
{
struct RawImage : public IRawImage
{
  int width{}, height{}, nChannels{};
  stbi_uc *pixels{};

  RawImage( const char *filename )
  {
    pixels = stbi_load( filename, &width, &height, &nChannels, 0 );
    if( !pixels )
      throw ErrorString( "failed to load image from file ", filename, "\n",
                         "because: ", stbi_failure_reason());
  }

  virtual ~RawImage() override
  {
    if( pixels )
      stbi_image_free( pixels );
  }

  virtual int getHeight() override { return height; }
  virtual int getNChannels() override { return nChannels; }
  virtual const unsigned char * getPixels() override { return pixels; }
  virtual int getWidth() override { return width; }
};
} // namespace

std::unique_ptr< IRawImage >
loadRawImage_StbImage( const char *filename )
{
  return std::make_unique< RawImage >( filename );
}
