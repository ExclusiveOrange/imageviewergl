#pragma once

struct IRawImage
{
  virtual ~IRawImage() = default;

  virtual int getHeight() = 0;
  virtual int getNChannels() = 0;
  virtual const unsigned char * getPixels() = 0;
  virtual int getWidth() = 0;
};