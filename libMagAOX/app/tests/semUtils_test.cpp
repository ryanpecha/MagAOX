/** \file semUtils_test.cpp
  * \brief Catch2 tests for the app semaphore utilities.
  *
  * History:
  */
#include "../../../tests/catch2/catch.hpp"

#include <semaphore.h>

#include <mx/sys/timeUtils.hpp>

#include "../../logger/logManager.hpp"

using namespace MagAOX::logger;

#include "../semUtils.hpp"
#include "../dev/semUtilsDerived.hpp"

namespace semUtils_test
{

struct testLogger
{
    template <typename logT, int retval = 0>
    static int log( const typename logT::messageT & )
    {
        return retval;
    }
};

struct semUtilsTest : public testLogger
{
    void waitTsRetVoid( timespec &ts, int sec, int nsec )
    {
        XWC_SEM_WAIT_TS_RETVOID( ts, sec, nsec );
    }

    int waitTs( timespec &ts, int sec, int nsec )
    {
        XWC_SEM_WAIT_TS( ts, sec, nsec );
        return 0;
    }

    int timedwaitLoop( sem_t &sem, timespec &ts, int maxIters )
    {
        int iters = 0;
        while( iters < maxIters )
        {
            ++iters;
            XWC_SEM_TIMEDWAIT_LOOP( sem, ts )
            return iters;
        }
        return -1;
    }

    int flush( sem_t &sem )
    {
        XWC_SEM_FLUSH( sem );
        return 0;
    }
};

template <class derivedT>
struct semUtilsDerivedMixin
{
    void waitTsRetVoidDerived( timespec &ts, int sec, int nsec )
    {
        XWC_SEM_WAIT_TS_RETVOID_DERIVED( ts, sec, nsec );
    }

    int waitTsDerived( timespec &ts, int sec, int nsec )
    {
        XWC_SEM_WAIT_TS_DERIVED( ts, sec, nsec );
        return 0;
    }

    int timedwaitLoopDerived( sem_t &sem, timespec &ts, int maxIters )
    {
        int iters = 0;
        while( iters < maxIters )
        {
            ++iters;
            XWC_SEM_TIMEDWAIT_LOOP_DERIVED( sem, ts )
            return iters;
        }
        return -1;
    }

    int flushDerived( sem_t &sem )
    {
        XWC_SEM_FLUSH_DERIVED( sem );
        return 0;
    }
};

struct semUtilsDerivedTest : public testLogger, public semUtilsDerivedMixin<semUtilsDerivedTest>
{
};

} // namespace semUtils_test

using namespace semUtils_test;

SCENARIO( "Adding wait time to a timespec", "[semUtils]" )
{
    GIVEN( "a semUtilsTest object" )
    {
        semUtilsTest t;

        WHEN( "using XWC_SEM_WAIT_TS_RETVOID with a working clock" )
        {
            timespec before;
            clock_gettime( CLOCK_REALTIME, &before );

            timespec ts;
            t.waitTsRetVoid( ts, 5, 500 );

            REQUIRE( ts.tv_sec >= before.tv_sec + 5 );
            REQUIRE( ts.tv_nsec >= 0 );
            REQUIRE( ts.tv_nsec < 1000000000 );
        }

        WHEN( "using XWC_SEM_WAIT_TS with a working clock" )
        {
            timespec before;
            clock_gettime( CLOCK_REALTIME, &before );

            timespec ts;
            int rv = t.waitTs( ts, 3, 250 );

            REQUIRE( rv == 0 );
            REQUIRE( ts.tv_sec >= before.tv_sec + 3 );
            REQUIRE( ts.tv_nsec >= 0 );
            REQUIRE( ts.tv_nsec < 1000000000 );
        }
    }
}

SCENARIO( "Waiting on a semaphore in a standard loop", "[semUtils]" )
{
    GIVEN( "a semUtilsTest object and a semaphore" )
    {
        semUtilsTest t;
        sem_t sem;
        sem_init( &sem, 0, 0 );

        WHEN( "the semaphore is already posted" )
        {
            sem_post( &sem );

            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            ts.tv_sec += 1;

            int rv = t.timedwaitLoop( sem, ts, 1 );
            REQUIRE( rv == 1 );
        }

        WHEN( "the wait times out" )
        {
            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );

            errno = 0;
            int rv = t.timedwaitLoop( sem, ts, 1 );
            REQUIRE( rv == -1 );
            REQUIRE( errno == ETIMEDOUT );
        }

        WHEN( "sem_timedwait fails for a reason other than a timeout" )
        {
            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            ts.tv_nsec = 2000000000;

            errno = 0;
            int rv = t.timedwaitLoop( sem, ts, 1 );
            REQUIRE( rv == -1 );
            REQUIRE( errno == EINVAL );
        }

        sem_destroy( &sem );
    }
}

SCENARIO( "Flushing a semaphore", "[semUtils]" )
{
    GIVEN( "a semUtilsTest object and a semaphore" )
    {
        semUtilsTest t;
        sem_t sem;
        sem_init( &sem, 0, 0 );

        WHEN( "the semaphore has several pending posts" )
        {
            sem_post( &sem );
            sem_post( &sem );
            sem_post( &sem );

            int rv = t.flush( sem );
            REQUIRE( rv == 0 );

            int val = -1;
            sem_getvalue( &sem, &val );
            REQUIRE( val == 0 );
        }

        WHEN( "the semaphore has no pending posts" )
        {
            int rv = t.flush( sem );
            REQUIRE( rv == 0 );

            int val = -1;
            sem_getvalue( &sem, &val );
            REQUIRE( val == 0 );
        }

        sem_destroy( &sem );
    }
}

SCENARIO( "Adding wait time to a timespec, derived", "[semUtilsDerived]" )
{
    GIVEN( "a semUtilsDerivedTest object" )
    {
        semUtilsDerivedTest t;

        WHEN( "using XWC_SEM_WAIT_TS_RETVOID_DERIVED with a working clock" )
        {
            timespec before;
            clock_gettime( CLOCK_REALTIME, &before );

            timespec ts;
            t.waitTsRetVoidDerived( ts, 5, 500 );

            REQUIRE( ts.tv_sec >= before.tv_sec + 5 );
            REQUIRE( ts.tv_nsec >= 0 );
            REQUIRE( ts.tv_nsec < 1000000000 );
        }

        WHEN( "using XWC_SEM_WAIT_TS_DERIVED with a working clock" )
        {
            timespec before;
            clock_gettime( CLOCK_REALTIME, &before );

            timespec ts;
            int rv = t.waitTsDerived( ts, 3, 250 );

            REQUIRE( rv == 0 );
            REQUIRE( ts.tv_sec >= before.tv_sec + 3 );
            REQUIRE( ts.tv_nsec >= 0 );
            REQUIRE( ts.tv_nsec < 1000000000 );
        }
    }
}

SCENARIO( "Waiting on a semaphore in a standard loop, derived", "[semUtilsDerived]" )
{
    GIVEN( "a semUtilsDerivedTest object and a semaphore" )
    {
        semUtilsDerivedTest t;
        sem_t sem;
        sem_init( &sem, 0, 0 );

        WHEN( "the semaphore is already posted" )
        {
            sem_post( &sem );

            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            ts.tv_sec += 1;

            int rv = t.timedwaitLoopDerived( sem, ts, 1 );
            REQUIRE( rv == 1 );
        }

        WHEN( "the wait times out" )
        {
            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );

            errno = 0;
            int rv = t.timedwaitLoopDerived( sem, ts, 1 );
            REQUIRE( rv == -1 );
            REQUIRE( errno == ETIMEDOUT );
        }

        WHEN( "sem_timedwait fails for a reason other than a timeout" )
        {
            timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            ts.tv_nsec = 2000000000;

            errno = 0;
            int rv = t.timedwaitLoopDerived( sem, ts, 1 );
            REQUIRE( rv == -1 );
            REQUIRE( errno == EINVAL );
        }

        sem_destroy( &sem );
    }
}

SCENARIO( "Flushing a semaphore, derived", "[semUtilsDerived]" )
{
    GIVEN( "a semUtilsDerivedTest object and a semaphore" )
    {
        semUtilsDerivedTest t;
        sem_t sem;
        sem_init( &sem, 0, 0 );

        WHEN( "the semaphore has several pending posts" )
        {
            sem_post( &sem );
            sem_post( &sem );
            sem_post( &sem );

            int rv = t.flushDerived( sem );
            REQUIRE( rv == 0 );

            int val = -1;
            sem_getvalue( &sem, &val );
            REQUIRE( val == 0 );
        }

        WHEN( "the semaphore has no pending posts" )
        {
            int rv = t.flushDerived( sem );
            REQUIRE( rv == 0 );

            int val = -1;
            sem_getvalue( &sem, &val );
            REQUIRE( val == 0 );
        }

        sem_destroy( &sem );
    }
}
