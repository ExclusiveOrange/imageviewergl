#pragma once

template< typename R, typename ... Vs >
struct IFunctor
{
  virtual ~IFunctor() = default;
  virtual R operator()( Vs... ) = 0;
};
