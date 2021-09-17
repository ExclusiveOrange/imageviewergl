#pragma once

#include "IFunctor.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace detail
{
  namespace makeUniqueFunctor
  {
    // Thanks to https://stackoverflow.com/a/39717241
    // for their templates which deduce various information about functions
    // including lambda functions, on which this solution is based.
    // Mostly I wanted to deduce the IFunctor template arguments so that
    // the user of makeUniqueFunctor doesn't have to redundantly specify them.
    // I've stripped some of their solution to only what I need here.

    template< typename R, typename ...Args >
    struct final_type_helper
    {
      using type = ::IFunctor< R, Args... >;
    };

    template< typename >
    struct const_or_nonconst_dispatch;

    template< typename R, typename C, typename ...Args >
    struct const_or_nonconst_dispatch< R ( C::* )( Args... ) >  // non-const methods
        : public final_type_helper< R, Args... >
    {
    };

    template< typename R, typename C, typename ...Args >
    struct const_or_nonconst_dispatch< R ( C::* )( Args... ) const > // const methods
        : public final_type_helper< R, Args... >
    {
    };

    // These next two overloads do the initial task of separating
    // plain function pointers for functors with ::operator()
    template< typename R, typename ...Args >
    final_type_helper< R, Args... >
    type_helper_function( R (*)( Args... ));

    template< typename F, typename ..., typename MemberType = decltype( &F::operator()) >
    const_or_nonconst_dispatch< MemberType >
    type_helper_function( F );

    template< typename T >
    struct IFunctor_type : public decltype( type_helper_function( std::declval< T >()))
    {
    };

    template< typename F >
    using IFunctor_t = typename IFunctor_type< F >::type;
  }
}

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
template< typename ... Vs, typename F, typename R = std::result_of_t< F( Vs... ) >>
std::unique_ptr< detail::makeUniqueFunctor::IFunctor_t< F >>
makeUniqueFunctor( F &&f )
{
  class Ftor : public IFunctor< R, Vs... >
  {
    std::remove_reference_t< F > f;
  public:
    Ftor( F &&f ) : f{ std::move( f ) } {}
    virtual ~Ftor() override = default;
    virtual R operator()( Vs &&... vs ) override { return f( std::forward< Vs >( vs )... ); }
  };

  return std::make_unique< Ftor >( std::move( f ));
}
