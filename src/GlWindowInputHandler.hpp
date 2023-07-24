#pragma once

struct GlWindowInputHandler
{
  virtual ~GlWindowInputHandler() = default;

  virtual void onCursorPosition(double xPos, double yPos) {} // top left = (0, 0)
  virtual void onKeyDown(int key, int scancode, int mods) {}
  virtual void onKeyRepeat(int key, int scancode, int mods) {}
  virtual void onKeyUp(int key, int scancode, int mods) {}
  virtual void onMouseDown(int button, int mods) {}
  virtual void onMouseUp(int button, int mods) {}
  virtual void onScroll(double xAmount, double yAmount) {}
};