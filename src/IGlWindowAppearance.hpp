#pragma once

#include <string>

struct IGlWindowAppearance
{
  virtual ~IGlWindowAppearance() = default;

  struct XYf { float x, y; };
  virtual XYf getContentScale() = 0;

  virtual void setContentAspectRatio( int numer, int denom ) = 0;
  virtual void setContentSize( int w, int h ) = 0;
  virtual void setTitle( std::string ) = 0;
};
