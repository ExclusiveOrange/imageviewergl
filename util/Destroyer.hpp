#pragma once

#include <functional>

struct Destroyer
{
  const std::function< void( void ) > fnOnDestroy;

  ~Destroyer() { fnOnDestroy(); }
};
