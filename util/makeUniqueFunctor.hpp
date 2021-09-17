#pragma once

#include "IFunctor.hpp"

#include <memory>
#include <type_traits>
#include <utility>


// Vs is zero or more input types - I'm not sure how to deduce these so you must specify them
// F is the deduced function type - do not set it
// R is the deduced return type - do not set it
//
// Examples:
//   auto floatToInt = makeUniqueFunctor< float >( []( float x ){ return (int)x; });
//   int i = (*floatToInt)( 5.1f );
//   auto two = makeUniqueFunctor( []{ return 2; });
//   int j = (*two)();
//   auto add = makeUniqueFunctor( []( float a, int b ){ return a + b; });
//   float sum = (*add)(7.6f, 3);
//
template< typename ... Args, typename F, typename R = std::result_of_t< F( Args... ) >>
std::unique_ptr< IFunctor_t< F >>
makeUniqueFunctor( F &&f )
{
  class Ftor : public IFunctor< R, Args... >
  {
    std::remove_reference_t< F > f;
  public:
    Ftor( F &&f ) : f{ std::move( f ) } {}
    virtual ~Ftor() override = default;
    virtual R operator()( Args &&... vs ) override { return f( std::forward< Args >( vs )... ); }
  };

  return std::make_unique< Ftor >( std::move( f ));
}
