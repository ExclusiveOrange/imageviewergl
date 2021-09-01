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
  void withLock( std::function< void( T & ) > fn )
  {
    std::unique_lock lk( m );
    fn( v );
  }

  void withLockThenNotify( std::function< void( T & ) > fn )
  {
    std::unique_lock lk( m );
    fn( v );
    cv.notify_one();
  }

  void waitThen(
      const std::function< bool( T & ) > & predicate,
      const std::function< void( T & ) > & then )
  {
    std::unique_lock lk( m );
    cv.wait( lk, [&,this]{ return predicate( v ); });
    then( v );
  }
};
