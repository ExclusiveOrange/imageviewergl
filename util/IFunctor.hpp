#pragma once

#include <utility>

template< typename R, typename ... Vs >
struct IFunctor
{
  virtual ~IFunctor() = default;
  virtual R operator()( Vs && ... ) = 0;
};

namespace detail
{
  namespace IFunctor
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
  }
}

template< typename F >
using IFunctor_t = typename detail::IFunctor::IFunctor_type< F >::type;
