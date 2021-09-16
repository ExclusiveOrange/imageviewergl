#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

template< class T >
class Mutexed
{
  T v;
  std::mutex m;
  std::condition_variable cv;

public:
  template< typename F, typename ... Args >
  void withLock( F && f, Args &&... args )
  {
    std::unique_lock lk( m );
    f( v, std::forward< Args >( args )... );
  }

  template< typename F, typename ... Args >
  void withLockThenNotify( F && f, Args &&... args )
  {
    std::unique_lock lk( m );
    f( v, std::forward< Args >( args )... );
    cv.notify_one();
  }

  template< typename Predicate, typename Then, typename ... ThenArgs >
  auto waitThen(
      Predicate && predicate,
      Then && then,
      ThenArgs &&... thenArgs )
  {
    std::unique_lock lk( m );
    cv.wait( lk, std::bind( predicate, std::cref( v )));
    return then( v, std::forward< ThenArgs >( thenArgs )... );
  }
};
