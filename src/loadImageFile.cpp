#include "ErrorString.hpp"
#include "loadImageFile.hpp"

#include <stb_image.h>

namespace
{
  struct RawImage : IRawImage, ImageDimensions
  {
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

    virtual ImageDimensions getDimensions() override { return *this; }
    virtual const unsigned char *getPixels() override { return pixels; }
  };
} // namespace

std::unique_ptr< IRawImage >
loadImageFile( const char *filename )
{
  return std::make_unique< RawImage >( filename );
}
