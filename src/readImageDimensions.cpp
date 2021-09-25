#include "readImageDimensions.hpp"

#include <stb_image.h>

ImageDimensions
readImageDimensions( const char *filename )
{
  ImageDimensions dimensions;

  stbi_info(
      filename,
      &dimensions.width,
      &dimensions.height,
      &dimensions.nChannels );

  return dimensions;
}
