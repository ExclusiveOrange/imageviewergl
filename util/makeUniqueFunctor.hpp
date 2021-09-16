#pragma once

#include "IFunctor.hpp"

#include <memory>

// R is return type
// Vs is zero or more input types
// F is deduced by the actual function type - do not set it
template< typename R, typename ... Vs, typename F >
std::unique_ptr< IFunctor< R, Vs... >>
makeUniqueFunctor( F && f )
{
  class Ftor : public IFunctor< R, Vs... >
  {
    std::remove_reference_t< F > f;
  public:
    Ftor( F && f ) : f{ std::move( f ) } {}
    virtual ~Ftor() override = default;
    virtual R operator()( Vs && ... vs ) override { return f( std::forward< Vs >( vs )... ); }
  };

  return std::make_unique< Ftor >( std::move( f ));
}
