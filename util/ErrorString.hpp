#pragma once

#include "toString.hpp"

#include <stdexcept>

struct ErrorString : public std::runtime_error
{
  ErrorString() = delete;

  template< class ... Ts >
  explicit
  ErrorString( const Ts &... vs )
      : std::runtime_error( toString( vs... )) {}
};
