#pragma once

#include <functional>

struct IGlWindow
{
  virtual ~IGlWindow() = default;

  virtual void enterEventLoop() = 0;

  struct XYf { float x, y; };
  virtual XYf getContentScale() = 0;

  virtual void hide() = 0;

  virtual void setRenderFunction( std::function< void( void ) > ) = 0;

  virtual void setContentAspectRatio( int numer, int denom ) = 0;

  virtual void setContentSize( int width, int height ) = 0;

  virtual void setTitle( const char * ) = 0;

  virtual void show() = 0;
};
