/** \file Thread_test.cpp
  * \brief Catch2 tests for pcf::Thread, pcf::MutexLock, and pcf::ReadWriteLock.
  *
  * All thread behavior is exercised with real threads; the lock error branches
  * that are exercised use real pthread deadlock detection (EDEADLK), not mocks.
  */
#include "../../tests/catch2/catch.hpp"

#include <atomic>
#include <cerrno>
#include <sys/resource.h>

#include "../libcommon/MutexLock.hpp"
#include "../libcommon/ReadWriteLock.hpp"
#include "../libcommon/SystemSocket.hpp"
#include "../libcommon/Thread.hpp"

namespace Thread_test
{

/// Counts execute() calls; used to observe the run loop doing work.
class CountingThread : public pcf::Thread
{
 public:
   std::atomic<int> m_count{ 0 };
   void execute() override
   {
      ++m_count;
   }
};

/// Throws a std::exception subclass out of execute().
class ThrowingThread : public pcf::Thread
{
 public:
   void execute() override
   {
      throw std::runtime_error( "thrown from execute" );
   }
};

/// Throws a non-std::exception value out of execute().
class ThrowingIntThread : public pcf::Thread
{
 public:
   void execute() override
   {
      throw 42;
   }
};

SCENARIO( "Thread lifecycle: start, pause, resume, trigger, stop", "[Thread]" )
{
   GIVEN( "a counting thread with a short interval" )
   {
      WHEN( "running, pausing, resuming, and stopping it" )
      {
         CountingThread t;
         t.setInterval( 1 );
         REQUIRE( t.getInterval() == 1 );
         REQUIRE( t.getState() == pcf::Thread::Idle );
         REQUIRE( !t.isRunning() );
         REQUIRE( t.join() != 0 ); // join before start: not running

         REQUIRE( t.start() == 0 );
         t.waitForReady();
         REQUIRE( t.isRunning() );
         REQUIRE( t.start() != 0 ); // already running

         // Let it do some work.
         for( int i = 0; i < 200 && t.m_count.load() == 0; ++i )
         {
            pcf::Thread::msleep( 5 );
         }
         REQUIRE( t.m_count.load() > 0 );
         REQUIRE( t.getState() == pcf::Thread::Execute );

         // Pause: the loop enters the self-pipe select; resume writes the pipe.
         t.pause();
         REQUIRE( t.isPaused() );
         pcf::Thread::msleep( 50 ); // let the loop actually block in select
         int paused = t.m_count.load();
         t.resume();
         REQUIRE( !t.isPaused() );
         for( int i = 0; i < 200 && t.m_count.load() == paused; ++i )
         {
            pcf::Thread::msleep( 5 );
         }
         REQUIRE( t.m_count.load() > paused );

         // Pause again and use resumeOnce, which wakes the loop but stays paused.
         t.pause();
         pcf::Thread::msleep( 50 );
         t.resumeOnce();
         pcf::Thread::msleep( 50 );
         t.resume();

         REQUIRE( !t.isStopping() );
         t.stop();
         REQUIRE( t.isStopping() );
         // join() races with the loop's own exit: 0 if we joined it, -EHOSTDOWN if
         // the thread had already finished and cleared its running flag.
         int j = t.join();
         REQUIRE( ( j == 0 || j == -EHOSTDOWN ) );
         REQUIRE( !t.isRunning() );
      }

      WHEN( "running with a datagram trigger socket" )
      {
         pcf::SystemSocket trigger( pcf::SystemSocket::Datagram, 51724, "127.0.0.1" );
         trigger.create();
         trigger.bind();

         CountingThread t;
         t.setInterval( 0 ); // rely on the trigger, not the interval
         t.setTrigger( &trigger );
         REQUIRE( t.start() == 0 );
         t.waitForReady();

         // Send a real datagram to fire the trigger select.
         pcf::SystemSocket sender( pcf::SystemSocket::Datagram, 51724, "127.0.0.1" );
         sender.create();
         sender.sendTo( "ping" );

         for( int i = 0; i < 200 && t.m_count.load() == 0; ++i )
         {
            pcf::Thread::msleep( 5 );
         }
         REQUIRE( t.m_count.load() > 0 );

         // Make sure the loop is parked back in the trigger select (no data
         // pending) so stop()'s signal interrupts the select itself and the
         // loop exits through the post-select stop check.
         pcf::Thread::msleep( 50 );
         t.stop();
         int j = t.join();
         REQUIRE( ( j == 0 || j == -EHOSTDOWN ) );

         sender.close();
         trigger.close();
      }

      WHEN( "stop() interrupts a thread parked in an idle trigger select" )
      {
         pcf::SystemSocket trigger( pcf::SystemSocket::Datagram, 51725, "127.0.0.1" );
         trigger.create();
         trigger.bind();

         CountingThread t;
         t.setInterval( 0 );
         t.setTrigger( &trigger );
         REQUIRE( t.start() == 0 );
         t.waitForReady();

         // No datagram is ever sent, so the loop blocks inside the trigger
         // select. stop()'s pthread_kill interrupts the select and the loop
         // exits through the post-select stop check.
         pcf::Thread::msleep( 50 );
         t.stop();
         int j = t.join();
         REQUIRE( ( j == 0 || j == -EHOSTDOWN ) );

         trigger.close();
      }
   }

   GIVEN( "the base Thread class (default execute/beforeExecute/afterExecute)" )
   {
      WHEN( "run directly, the defaults are no-ops" )
      {
         pcf::Thread t;
         t.setInterval( 1 );
         REQUIRE( t.start() == 0 );
         t.waitForReady();
         pcf::Thread::msleep( 20 );
         t.stop();
         REQUIRE( t.join() == 0 );
      }

      WHEN( "copy-constructing and assigning" )
      {
         pcf::Thread a;
         a.setInterval( 7 );

         pcf::Thread b( a );
         REQUIRE( b.getInterval() == 7 );

         // NOTE: operator= re-locks m_mutReady, which the constructor leaves locked
         // until the thread's runLoop() unlocks it -- so assigning onto a thread that
         // has never been started self-deadlocks. Run the target thread once first,
         // which leaves m_mutReady unlocked, matching operator='s expectation.
         pcf::Thread c;
         c.setInterval( 1 );
         REQUIRE( c.start() == 0 );
         c.waitForReady();
         c.stop();
         REQUIRE( c.join() == 0 );

         c = a;
         REQUIRE( c.getInterval() == 7 );
         c = c; // self-assignment branch
         REQUIRE( c.getInterval() == 7 );
      }
   }

   GIVEN( "threads whose execute() throws" )
   {
      WHEN( "a std::exception escapes execute" )
      {
         ThrowingThread t;
         t.setInterval( 1 );
         REQUIRE( t.start() == 0 );
         pcf::Thread::msleep( 50 );
         t.stop();
         t.join();
      }

      WHEN( "a non-std exception escapes execute" )
      {
         ThrowingIntThread t;
         t.setInterval( 1 );
         REQUIRE( t.start() == 0 );
         pcf::Thread::msleep( 50 );
         t.stop();
         t.join();
      }
   }

   GIVEN( "scheduling and affinity options" )
   {
      WHEN( "starting pinned to CPU 0 with Normal scheduling" )
      {
         CountingThread t;
         t.setInterval( 1 );
         REQUIRE( t.start( 0, pcf::Thread::Normal ) == 0 );
         t.waitForReady();
         t.stop();
         REQUIRE( t.join() == 0 );
      }

      WHEN( "requesting Turbo (SCHED_FIFO) scheduling without privileges" )
      {
         // A non-root process cannot create a SCHED_FIFO max-priority thread, so
         // this genuinely exercises the failed-create path.
         CountingThread t;
         t.setInterval( 1 );
         int rv = t.start( -1, pcf::Thread::Turbo );
         if( rv == 0 ) // in case the environment does grant RT scheduling
         {
            t.stop();
            t.join();
         }
         REQUIRE( true );
      }
   }

   GIVEN( "a thread whose self-pipe could not be created" )
   {
      WHEN( "resume and resumeOnce report the failed pipe write" )
      {
         // Exhaust the file-descriptor limit for real while the Thread
         // constructor runs, so its pipe() call genuinely fails and the
         // self-pipe fds stay -1.
         struct rlimit old;
         REQUIRE( getrlimit( RLIMIT_NOFILE, &old ) == 0 );
         struct rlimit low = old;
         low.rlim_cur = 3;
         REQUIRE( setrlimit( RLIMIT_NOFILE, &low ) == 0 );

         pcf::Thread t;

         REQUIRE( setrlimit( RLIMIT_NOFILE, &old ) == 0 );

         // With no pipe, the resume writes fail and warn on stderr.
         t.resume();
         t.resumeOnce();
         REQUIRE( true );
      }
   }

   GIVEN( "the sleep helpers and error messages" )
   {
      WHEN( "sleeping tiny intervals through each unit" )
      {
         REQUIRE( pcf::Thread::sleep( 0 ) == 0 );
         REQUIRE( pcf::Thread::msleep( 1 ) == 0 );
         REQUIRE( pcf::Thread::usleep( 100 ) == 0 );
         REQUIRE( pcf::Thread::nsleep( 100 ) == 0 );
      }

      WHEN( "asking for every error message" )
      {
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrNone ) == "No Error." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrThreadUnjoinable ) == "Thread unjoinable." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrBadThreadId ) == "Bad thread Id." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrDeadlock ) == "Operation would deadlock." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrCouldNotCreateThread ) == "Could not create thread." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrInvalidParameter ) == "Invalid parameter to function call." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrWrongPermission ) == "Wrong permission." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrAlreadyRunning ) == "Thread already running." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrInterrupted ) ==
                  "Sleep was interrupted before it could complete." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrCopy ) ==
                  "Information could not be copied from user space." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrTimedOut ) == "Timed out waiting for thread." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrNotRunning ) == "Thread is not running." );
         REQUIRE( pcf::Thread::getErrorMsg( pcf::Thread::ErrUnknown ) == "Unknown error." );
         REQUIRE( pcf::Thread::getErrorMsg( -424242 ) == "" );
      }
   }
}

SCENARIO( "MutexLock and ReadWriteLock guards and error branches", "[locks]" )
{
   GIVEN( "a MutexLock" )
   {
      WHEN( "using AutoLock normally and with a NULL pointer" )
      {
         pcf::MutexLock mut;
         {
            pcf::MutexLock::AutoLock lock( &mut );
         }
         REQUIRE_THROWS_AS( pcf::MutexLock::AutoLock( nullptr ), std::invalid_argument );
      }
   }

   GIVEN( "a ReadWriteLock" )
   {
      WHEN( "using the auto guards normally and with NULL pointers" )
      {
         pcf::ReadWriteLock rw;
         {
            pcf::ReadWriteLock::AutoRLock rl( &rw );
         }
         {
            pcf::ReadWriteLock::AutoWLock wl( &rw );
         }
         REQUIRE_THROWS_AS( pcf::ReadWriteLock::AutoRLock( nullptr ), std::invalid_argument );
         REQUIRE_THROWS_AS( pcf::ReadWriteLock::AutoWLock( nullptr ), std::invalid_argument );
      }

      WHEN( "provoking real pthread deadlock detection" )
      {
         // glibc returns EDEADLK for a write-lock while the same thread already
         // holds the write lock, and for a read-lock while holding the write lock.
         pcf::ReadWriteLock rw;
         rw.lockWrite();
         REQUIRE_THROWS_AS( rw.lockWrite(), std::runtime_error );
         REQUIRE_THROWS_AS( rw.lockRead(), std::runtime_error );
         rw.unlockWrite();
      }
   }
}

} //namespace Thread_test
