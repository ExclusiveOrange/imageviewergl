#pragma once

#include "IFunctor.hpp"

#include <memory>
#include <type_traits>
#include <utility>

//==============================================================================
// function at bottom
//==============================================================================

namespace detail::makeUniqueFunctor
{
  // This template machinery helps deduce types for the makeUniqueFunctor function at the bottom.
  // Thanks to https://stackoverflow.com/a/39646799 for showing how to do this; particularly how
  // it is necessary to split by const / non-const method (functor) specialization.

  template< class F, class R, class ... Args >
  struct functor : ::IFunctor<R, Args...>
  {
    std::remove_reference_t<F> f;

    explicit
    functor( F &&f ) : f{ std::forward<F>( f ) } {}

    ~functor() override = default;

    R operator()( Args &&... args ) override { return f( std::forward<Args>( args )... ); }
  };

  template< class R, class ...Args >
  struct final_type_helper
  {
    using interface_t = ::IFunctor<R, Args...>;

    template< class F >
    using impl_t = functor<F, R, Args...>;
  };

  //------------------------------------------------------------------------------

  template< typename >
  struct const_or_nonconst_dispatch;

  // non-const method
  template< class R, class C, class ... Args >
  struct const_or_nonconst_dispatch<R ( C::* )( Args... )>
      : public final_type_helper<R, Args...>
  {
  };

  // const method
  template< class R, class C, class ... Args >
  struct const_or_nonconst_dispatch<R ( C::* )( Args... ) const>
      : public final_type_helper<R, Args...>
  {
  };

  //------------------------------------------------------------------------------

  // function
  template< class R, class ... Args >
  final_type_helper<R, Args...>
  function_or_functor_dispatch( R (*)( Args... ));

  // functor or lambda
  template< class F, class ..., class MemberType = decltype( &F::operator()) >
  const_or_nonconst_dispatch<MemberType>
  function_or_functor_dispatch( F );

  //------------------------------------------------------------------------------

  template< class F >
  struct functor_type : decltype( function_or_functor_dispatch( std::declval<F>())) {};
}

//==============================================================================

template< typename F >
std::unique_ptr<typename detail::makeUniqueFunctor::functor_type<F>::interface_t>
makeUniqueFunctor( F &&f )
{
  using impl_t = typename detail::makeUniqueFunctor::functor_type<F>::template impl_t<F>;
  return std::make_unique<impl_t>( std::forward<F>( f ));
}