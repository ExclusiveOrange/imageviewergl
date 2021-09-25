#pragma once

#include "IGlWindowAppearance.hpp"

struct IGlWindow : IGlWindowAppearance
{
  virtual ~IGlWindow() = default;
  virtual void enterEventLoop() = 0;
  virtual void hide() = 0;
  virtual void show() = 0;
};
