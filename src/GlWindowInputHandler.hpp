#pragma once

struct GlWindowInputHandler
{
  virtual ~GlWindowInputHandler() = default;

  virtual void onCursorPosition( double xPos, double yPos ) {} // top left = (0, 0)
  virtual void onScroll( double xAmount, double yAmount ) {}
};
