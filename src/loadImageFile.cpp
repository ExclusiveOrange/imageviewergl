#include "ErrorString.hpp"
#include "loadImageFile.hpp"

#include <stb_image.h>

namespace
{
  struct RawImage : IRawImage, ImageDimensions
  {
    stbi_uc *pixels{};

    explicit
    RawImage( const char *filename )
    : pixels{ stbi_load( filename, &width, &height, &nChannels, 0 )}
    {
      if( !pixels )
        throw ErrorString( "failed to load image from file ", filename, "\n",
                           "because: ", stbi_failure_reason());
    }

    ~RawImage() override
    {
      if( pixels ) // 2021.09.27: CLion warns that the condition is always false: this is a CLion or Clang bug; debugging with a breakpoint shows the condition is not false
        stbi_image_free( pixels );
    }

    ImageDimensions getDimensions() override { return *this; /* NOLINT(cppcoreguidelines-slicing) */ }
    const unsigned char *getPixels() override { return pixels; }
  };
} // namespace

std::unique_ptr< IRawImage >
loadImageFile( const char *filename )
{
  return std::make_unique< RawImage >( filename );
}
