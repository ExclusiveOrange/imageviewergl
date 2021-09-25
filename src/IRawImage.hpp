#pragma once

#include "ImageDimensions.hpp"

struct IRawImage
{
  virtual ~IRawImage() = default;

  virtual ImageDimensions getDimensions() = 0;
  virtual const unsigned char * getPixels() = 0;
};