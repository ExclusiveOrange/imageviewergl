#pragma once

#include "GlWindowInputHandler.hpp"
#include "IGlWindowAppearance.hpp"

#include <memory>

struct IGlWindow : IGlWindowAppearance
{
  ~IGlWindow() override = default;
  virtual void enterEventLoop( std::shared_ptr< GlWindowInputHandler > ) = 0;
  virtual void hide() = 0;
  virtual void show() = 0;
};
