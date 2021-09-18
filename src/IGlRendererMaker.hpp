#pragma once

#include "IGlRenderer.hpp"
#include "IGlWindowAppearance.hpp"

#include <memory>

struct IGlRendererMaker
{
  virtual ~IGlRendererMaker() = default;

  virtual
  std::unique_ptr< IGlRenderer >
  makeGlRenderer( IGlWindowAppearance & ) = 0;
};
