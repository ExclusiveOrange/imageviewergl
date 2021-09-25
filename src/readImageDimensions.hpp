#pragma once

#include "ImageDimensions.hpp"

// gets the dimensions of an image in a file
// should be quicker than loading the image for some formats
// because only a bit of the file need be read

ImageDimensions
readImageDimensions( const char *filename )
noexcept( false ); // throws ErrorString
