/** \file TimeStamp_test.cpp
  * \brief Catch2 tests for pcf::TimeStamp (INDI/libcommon/TimeStamp.cpp).
  *
  * Uses the fixed instant 2007-06-24T19:38:12.234000Z (a Sunday; unix epoch
  * 1182713892) so every formatted output can be asserted exactly.
  */
#include "../../tests/catch2/catch.hpp"

#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>

#include "../libcommon/TimeStamp.hpp"

using pcf::TimeStamp;

namespace TimeStamp_test
{

static TimeStamp fixed()
{
   timeval tv;
   tv.tv_sec  = 1182713892; // 2007-06-24T19:38:12Z, a Sunday
   tv.tv_usec = 234000;
   return TimeStamp( tv );
}

SCENARIO( "TimeStamp construction, assignment, and arithmetic", "[TimeStamp]" )
{
   GIVEN( "the various constructors and assignments" )
   {
      WHEN( "constructing each way" )
      {
         TimeStamp tsNow; // default = now
         REQUIRE( tsNow.getTimeVal().tv_sec > 1182713892 );

         TimeStamp tsFixed = fixed();
         REQUIRE( tsFixed.getTimeVal().tv_sec == 1182713892 );
         REQUIRE( tsFixed.getTimeVal().tv_usec == 234000 );

         TimeStamp tsMillis( 1500 ); // millisecond constructor
         REQUIRE( tsMillis.getTimeVal().tv_sec == 1 );
         REQUIRE( tsMillis.getTimeVal().tv_usec == 500000 );

         TimeStamp tsYmd( 2007, 6, 24, 19, 38, 12 );
         REQUIRE( tsYmd.getTimeVal().tv_sec == 1182713892 );

         // With TZ set in the environment, local_timegm() takes its save-and-
         // restore branch for real.
         setenv( "TZ", "America/Phoenix", 1 );
         tzset();
         TimeStamp tsYmdTz( 2007, 6, 24, 19, 38, 12 );
         REQUIRE( tsYmdTz.getTimeVal().tv_sec == 1182713892 );
         unsetenv( "TZ" );
         tzset();

         TimeStamp tsCopy( tsFixed );
         REQUIRE( tsCopy == tsFixed );

         TimeStamp tsAssigned;
         tsAssigned = tsFixed;
         REQUIRE( tsAssigned == tsFixed );
         tsAssigned = tsAssigned; // self-assignment branch
         REQUIRE( tsAssigned == tsFixed );

         timeval tv;
         tv.tv_sec  = 12;
         tv.tv_usec = 34;
         TimeStamp tsTv;
         tsTv = tv;
         REQUIRE( tsTv.getTimeVal().tv_sec == 12 );

         TimeStamp tsMs2;
         tsMs2 = 2750; // millisecond assignment
         REQUIRE( tsMs2.getTimeVal().tv_sec == 2 );
         REQUIRE( tsMs2.getTimeVal().tv_usec == 750000 );
      }
   }

   GIVEN( "timestamps to add, subtract, and compare" )
   {
      TimeStamp a( 1500 ); // 1.5 s
      TimeStamp b( 2750 ); // 2.75 s

      WHEN( "subtracting with and without the microsecond borrow" )
      {
         TimeStamp d1 = b - a; // 0.75s + 0.25s: usec 750000-500000 >= 0
         REQUIRE( d1.getTimeVal().tv_sec == 1 );
         REQUIRE( d1.getTimeVal().tv_usec == 250000 );

         TimeStamp d2 = a - b; // usec 500000-750000 < 0: borrow
         REQUIRE( d2.getTimeVal().tv_usec == 750000 );
      }

      WHEN( "adding with and without the microsecond carry" )
      {
         TimeStamp s1 = a + b; // usec 500000+750000 >= 1e6: carry
         REQUIRE( s1.getTimeVal().tv_sec == 4 );
         REQUIRE( s1.getTimeVal().tv_usec == 250000 );

         TimeStamp small( 100 ); // 0.1 s
         TimeStamp s2 = a + small; // usec 500000+100000 < 1e6: no carry
         REQUIRE( s2.getTimeVal().tv_sec == 1 );
         REQUIRE( s2.getTimeVal().tv_usec == 600000 );
      }

      WHEN( "comparing" )
      {
         REQUIRE( a < b );
         REQUIRE( !( b < a ) );
         REQUIRE( a <= b );
         REQUIRE( a <= a );
         REQUIRE( b > a );
         REQUIRE( !( a > b ) );
         REQUIRE( b >= a );
         REQUIRE( b >= b );
         REQUIRE( a == a );
         REQUIRE( !( a == b ) );
      }

      WHEN( "measuring elapsed time" )
      {
         REQUIRE( a.elapsedMillis( b ) == Approx( 1250.0 ) );
         REQUIRE( a.elapsedDays( b ) == Approx( 1250.0 / 86400000.0 ) );

         TimeStamp past( 0 );
         REQUIRE( past.intervalElapsedMillis( 0 ) == true ); // resets to now
         TimeStamp now2 = TimeStamp::now();
         REQUIRE( now2.intervalElapsedMillis( 1000000000 ) == false );
      }

      WHEN( "converting to scalar representations" )
      {
         REQUIRE( a.getMicros() == Approx( 1500000.0 ) );
         REQUIRE( a.getMillis() == Approx( 1500.0 ) );
         REQUIRE( a.getDays() == Approx( 1500.0 / 86400000.0 ) );
         REQUIRE( a.getMicrosStr() == "1.5e+06" );
         REQUIRE( a.getMillisStr() == "1500" );

         std::ostringstream oss;
         oss << fixed(); // operator<<
         REQUIRE( oss.str() == "Sun Jun 24 19:38:12.234 2007" );
      }
   }
}

SCENARIO( "TimeStamp formatted output and field extraction", "[TimeStamp]" )
{
   GIVEN( "the fixed instant 2007-06-24T19:38:12.234000Z" )
   {
      TimeStamp ts = fixed();

      WHEN( "formatting" )
      {
         REQUIRE( ts.getFormattedIsoTimeStr() == "193812" );
         REQUIRE( ts.getFormattedStr() == "Sun Jun 24 19:38:12.234 2007" );
         REQUIRE( ts.getFormattedIso8601Str() == "2007-06-24T19:38:12.234000Z" );
         REQUIRE( ts.getFormattedIsoDateStr() == "20070624" );
      }

      WHEN( "extracting each field" )
      {
         REQUIRE( ts.getYear() == 2007 );
         REQUIRE( ts.getYearMonth() == 6 );
         REQUIRE( ts.getMonthDay() == 24 );
         REQUIRE( ts.getDayHour() == 19 );
         REQUIRE( ts.getHourMinute() == 38 );
         REQUIRE( ts.getMinuteSecond() == 12 );
         REQUIRE( ts.getSecondMillisecond() == 234 );
      }

      WHEN( "incrementing and decrementing a day" )
      {
         TimeStamp t2 = ts;
         t2.incrementDay();
         REQUIRE( t2.getMonthDay() == 25 );
         t2.decrementDay();
         REQUIRE( t2.getMonthDay() == 24 );
      }

      WHEN( "round-tripping through MJD" )
      {
         double mjd = ts.getMJD();
         REQUIRE( mjd == Approx( 54275.8182 ).epsilon( 0.0001 ) );

         TimeStamp t3;
         t3.fromMJD( mjd );
         REQUIRE( t3.getTimeVal().tv_sec == ts.getTimeVal().tv_sec );
      }

      WHEN( "round-tripping through the ISO 8601 string" )
      {
         TimeStamp t4;
         t4.fromFormattedIso8601Str( "2007-06-24T19:38:12.234000Z" );
         REQUIRE( t4.getTimeVal().tv_sec == 1182713892 );
         REQUIRE( t4.getTimeVal().tv_usec == 234000 );
      }
   }

   GIVEN( "the month and weekday lookup tables" )
   {
      WHEN( "looking up month numbers from names" )
      {
         REQUIRE( TimeStamp::getMonthNumber( "January" ) == 1 );
         REQUIRE( TimeStamp::getMonthNumber( "feb" ) == 2 );
         REQUIRE( TimeStamp::getMonthNumber( "MAR" ) == 3 );
         REQUIRE( TimeStamp::getMonthNumber( "apr" ) == 4 );
         REQUIRE( TimeStamp::getMonthNumber( "may" ) == 5 );
         REQUIRE( TimeStamp::getMonthNumber( "jun" ) == 6 );
         REQUIRE( TimeStamp::getMonthNumber( "jul" ) == 7 );
         REQUIRE( TimeStamp::getMonthNumber( "aug" ) == 8 );
         REQUIRE( TimeStamp::getMonthNumber( "sep" ) == 9 );
         REQUIRE( TimeStamp::getMonthNumber( "oct" ) == 10 );
         REQUIRE( TimeStamp::getMonthNumber( "nov" ) == 11 );
         REQUIRE( TimeStamp::getMonthNumber( "dec" ) == 12 );
         REQUIRE( TimeStamp::getMonthNumber( "xyz" ) == -1 );
      }

      WHEN( "looking up weekday and month names from numbers" )
      {
         REQUIRE( TimeStamp::getWeekdayName( 1 ) == "Sun" );
         REQUIRE( TimeStamp::getWeekdayName( 2 ) == "Mon" );
         REQUIRE( TimeStamp::getWeekdayName( 3 ) == "Tue" );
         REQUIRE( TimeStamp::getWeekdayName( 4 ) == "Wed" );
         REQUIRE( TimeStamp::getWeekdayName( 5 ) == "Thu" );
         REQUIRE( TimeStamp::getWeekdayName( 6 ) == "Fri" );
         REQUIRE( TimeStamp::getWeekdayName( 7 ) == "Sat" );
         REQUIRE( TimeStamp::getWeekdayName( 0 ) == "???" );

         REQUIRE( TimeStamp::getMonthName( 1 ) == "Jan" );
         REQUIRE( TimeStamp::getMonthName( 2 ) == "Feb" );
         REQUIRE( TimeStamp::getMonthName( 3 ) == "Mar" );
         REQUIRE( TimeStamp::getMonthName( 4 ) == "Apr" );
         REQUIRE( TimeStamp::getMonthName( 5 ) == "May" );
         REQUIRE( TimeStamp::getMonthName( 6 ) == "Jun" );
         REQUIRE( TimeStamp::getMonthName( 7 ) == "Jul" );
         REQUIRE( TimeStamp::getMonthName( 8 ) == "Aug" );
         REQUIRE( TimeStamp::getMonthName( 9 ) == "Sep" );
         REQUIRE( TimeStamp::getMonthName( 10 ) == "Oct" );
         REQUIRE( TimeStamp::getMonthName( 11 ) == "Nov" );
         REQUIRE( TimeStamp::getMonthName( 12 ) == "Dec" );
         REQUIRE( TimeStamp::getMonthName( 0 ) == "???" );
      }
   }
}

} //namespace TimeStamp_test
