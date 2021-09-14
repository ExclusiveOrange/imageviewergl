#include "makeUniqueFunctor.hpp"

void f()
{
  // void( void )
  auto g = makeUniqueFunctor< void >( [] {} );

  (*g)();

  // int( void )
  auto h = makeUniqueFunctor< int >( [] { return 5; } );

  int z = (*h)();

  // int( float )
  auto ui = std::make_unique< int >( 5 );
  auto i = makeUniqueFunctor< int, float >( [ui = std::move( ui )]( float f )mutable { return (int)f; } );

  float s = (*i)( 5 );
}