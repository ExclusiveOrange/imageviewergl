#pragma once

struct IGlRenderer
{
  virtual ~IGlRenderer() = default;

  virtual void render() = 0;
};
