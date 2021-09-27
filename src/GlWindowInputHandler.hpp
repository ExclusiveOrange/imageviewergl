#pragma once

struct GlWindowInputHandler
{
  virtual ~GlWindowInputHandler() = default;

  virtual void onScroll( double xAmount, double yAmount ) {}
};
