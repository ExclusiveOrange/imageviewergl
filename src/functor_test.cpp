#include "makeUniqueFunctor.hpp"

void f()
{
  // void( void )
  auto g = makeUniqueFunctor( [] {} );

  (*g)();

  // int( void )
  auto h = makeUniqueFunctor( [] { return 5; } );

  int z = (*h)();

  // int( float )
  auto ui = std::make_unique< int >( 5 );
  auto i = makeUniqueFunctor< float >( [ui = std::move( ui )]( float f )mutable { return (int)f; } );

  float s = (*i)( 5 );

  {
    auto floatToInt = makeUniqueFunctor< float >( []( float x ) { return (int)x; } );
    int i = (*floatToInt)( 5.1f );
    auto two = makeUniqueFunctor( [] { return 2; } );
    int j = (*two)();
    auto add = makeUniqueFunctor< float, int >( []( float a, int b ) { return a + b; } );
    float sum = (*add)( 7.6f, 3 );
  }
}