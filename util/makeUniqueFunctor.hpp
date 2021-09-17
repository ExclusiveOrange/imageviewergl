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
//template< typename ... Args, typename F, typename R = std::result_of_t< F( Args... ) >>
//std::unique_ptr< IFunctor_t< F >>
//makeUniqueFunctor( F &&f )
//{
//  class Ftor : public IFunctor< R, Args... >
//  {
//    std::remove_reference_t< F > f;
//  public:
//    Ftor( F &&f ) : f{ std::move( f ) } {}
//    virtual ~Ftor() override = default;
//    virtual R operator()( Args &&... vs ) override { return f( std::forward< Args >( vs )... ); }
//  };
//
//  return std::make_unique< Ftor >( std::move( f ));
//}
//
namespace detail
{
  namespace makeUniqueFunctor
  {
    template< class F, class R, class ... Args >
    struct functor : ::IFunctor< R, Args... >
    {
      std::remove_reference_t< F > f;

      functor( F && f ) : f{ std::forward< F >( f )} {}
      virtual ~functor() override = default;
      virtual R operator()( Args &&... args ) override
      { return f( std::forward< Args >( args )... ); }
    };

    template< class R, class ...Args >
    struct final_type_helper
    {
      using type = ::IFunctor< R, Args... >;

      template< class F >
      using impl_type = functor< F, R, Args... >;
    };

    //------------------------------------------------------------------------------

    template< typename >
    struct const_or_nonconst_dispatch;

    template< class R, class C, class ... Args >
    struct const_or_nonconst_dispatch< R ( C::* )( Args... ) > // non-const methods
        : public final_type_helper< R, Args... >
    {
    };

    template< class R, class C, class ... Args >
    struct const_or_nonconst_dispatch< R ( C::* )( Args... ) const > // non-const methods
        : public final_type_helper< R, Args... >
    {
    };

    //------------------------------------------------------------------------------

    template< class R, class ... Args >
    final_type_helper< R, Args... >
    function_or_functor_dispatch( R (*)( Args... ));

    template< class F, class ..., class MemberType = decltype( &F::operator()) >
    const_or_nonconst_dispatch< MemberType >
    function_or_functor_dispatch( F );

    //------------------------------------------------------------------------------

    template< class F >
    struct ftor_type : decltype( function_or_functor_dispatch( std::declval< F >())) {};
  }
}

template< typename F >
std::unique_ptr< typename detail::makeUniqueFunctor::ftor_type< F >::type >
makeUniqueFunctor( F &&f )
{
  using ftor_t = typename detail::makeUniqueFunctor::ftor_type< F >::template impl_type< F >;
  return std::make_unique< ftor_t >( std::forward< F >( f ));
}