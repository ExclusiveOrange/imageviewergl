#pragma once

#include "NoCopy.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>

template< class T >
class Mutexed : NoCopy
{
  T v;
  std::mutex m;
  std::condition_variable cv;

public:
  template< typename ... Args >
  Mutexed( Args &&... args )
      : v{ std::forward< Args >( args )... } {}

  template< typename F, typename ... Args >
  void withLock( F &&f, Args &&... args )
  {
    std::unique_lock lk( m );
    std::forward< F >( f )( v, std::forward< Args >( args )... );
  }

  template< typename F, typename ... Args >
  void withLockThenNotify( F &&f, Args &&... args )
  {
    std::unique_lock lk( m );
    std::forward< F >( f )( v, std::forward< Args >( args )... );
    cv.notify_one();
  }

  template< typename F, typename ... Args >
  void withoutLock( F &&f, Args &&... args )
  {
    std::forward< F >( f )( v, std::forward< Args >( args )... );
  }

  template< typename Predicate, typename Then, typename ... ThenArgs >
  auto waitThen(
      Predicate &&predicate, // (const T &) -> bool
      Then &&then, // (T &, ThenArgs...) -> auto
      ThenArgs &&... thenArgs )
  {
    std::unique_lock lk( m );
    cv.wait( lk, std::bind( std::forward< Predicate >( predicate ), std::cref( v )));
    return std::forward< Then >( then )( v, std::forward< ThenArgs >( thenArgs )... );
  }
};
