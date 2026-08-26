/** \file flatlogcodes_test.cpp
  * \brief Catch2 tests for the flatlogs headers logHeader, logPriority, and timespecX.
  *
  * These are pure in-memory tests. The header encoding tests write into a heap buffer
  * and read it back. No files, sockets, or devices are involved.
  *
  * History:
  */
#include "../../tests/catch2/catch.hpp"

#include <cstring>

#include "../include/flatlogs/flatlogs.hpp"
#include "../include/flatlogs/logDefs.hpp"
#include "../include/flatlogs/logHeader.hpp"
#include "../include/flatlogs/logPriority.hpp"
#include "../include/flatlogs/logStdFormat.hpp"
#include "../include/flatlogs/timespecX.hpp"

namespace flatlogs_test
{

using namespace flatlogs;

// Verifies the three variable-length message size encodings of logHeader. It checks the
// size computed for each regime, then writes each encoding into a raw buffer and reads the
// length and header size back.
SCENARIO( "logHeader message-length encodings", "[flatlogs::logHeader]" )
{
   GIVEN( "intended message sizes in each of the three length regimes" )
   {
      WHEN( "the size fits in msgLen0 (short)" )
      {
         msgLenT len = 10;
         REQUIRE( logHeader::lenSize( len ) == sizeof( msgLen0T ) );
      }

      WHEN( "the size needs msgLen1 (medium)" )
      {
         msgLenT len = 300;
         REQUIRE( logHeader::lenSize( len ) == sizeof( msgLen0T ) + sizeof( msgLen1T ) );
      }

      WHEN( "the size needs msgLen2 (large)" )
      {
         msgLenT len = 70000;
         REQUIRE( logHeader::lenSize( len ) == sizeof( msgLen0T ) + sizeof( msgLen2T ) );
      }
   }

   GIVEN( "a raw log buffer with each length encoding written into it" )
   {
      bufferPtrT buf( new char[64], std::default_delete<char[]>() );
      memset( buf.get(), 0, 64 );

      WHEN( "a short length is set" )
      {
         REQUIRE( logHeader::msgLen( buf, 10 ) == 0 );
         REQUIRE( logHeader::msgLen0( buf ) == 10 );
         REQUIRE( logHeader::msgLen( buf ) == 10 );
         REQUIRE( logHeader::lenSize( buf ) == sizeof( msgLen0T ) );
         REQUIRE( logHeader::headerSize( buf ) ==
                  sizeof( logPrioT ) + sizeof( eventCodeT ) + sizeof( secT ) + sizeof( nanosecT ) +
                      sizeof( msgLen0T ) );
      }

      WHEN( "a medium length is set" )
      {
         REQUIRE( logHeader::msgLen( buf, 300 ) == 0 );
         REQUIRE( logHeader::msgLen1( buf ) == 300 );
         REQUIRE( logHeader::msgLen( buf ) == 300 );
         REQUIRE( logHeader::lenSize( buf ) == sizeof( msgLen0T ) + sizeof( msgLen1T ) );
      }

      WHEN( "a large length is set" )
      {
         REQUIRE( logHeader::msgLen( buf, 70000 ) == 0 );
         REQUIRE( logHeader::msgLen( buf ) == 70000 );
         REQUIRE( logHeader::lenSize( buf ) == sizeof( msgLen0T ) + sizeof( msgLen2T ) );
         REQUIRE( logHeader::headerSize( buf ) ==
                  sizeof( logPrioT ) + sizeof( eventCodeT ) + sizeof( secT ) + sizeof( nanosecT ) +
                      sizeof( msgLen0T ) + sizeof( msgLen2T ) );
      }
   }
}

// Verifies the priority to string table and the string to priority parser. The parser
// cases cover numeric input, every accepted name and prefix form, and rejected input.
SCENARIO( "logPriority string conversions", "[flatlogs::logPriority]" )
{
   GIVEN( "each log priority value" )
   {
      WHEN( "converting priorities to strings" )
      {
         logPrioT p;
         p = logPrio::LOG_EMERGENCY; REQUIRE( priorityString( p ) == "EMER" );
         p = logPrio::LOG_ALERT;     REQUIRE( priorityString( p ) == "ALRT" );
         p = logPrio::LOG_CRITICAL;  REQUIRE( priorityString( p ) == "CRIT" );
         p = logPrio::LOG_ERROR;     REQUIRE( priorityString( p ) == "ERR " );
         p = logPrio::LOG_WARNING;   REQUIRE( priorityString( p ) == "WARN" );
         p = logPrio::LOG_NOTICE;    REQUIRE( priorityString( p ) == "NOTE" );
         p = logPrio::LOG_INFO;      REQUIRE( priorityString( p ) == "INFO" );
         p = logPrio::LOG_DEBUG;     REQUIRE( priorityString( p ) == "DBG " );
         p = logPrio::LOG_DEBUG2;    REQUIRE( priorityString( p ) == "DBG2" );
         p = logPrio::LOG_DEFAULT;   REQUIRE( priorityString( p ) == "DEF?" );
         p = logPrio::LOG_TELEM;     REQUIRE( priorityString( p ) == "TELM" );
         p = 111;                    REQUIRE( priorityString( p ) == "UNK?" );
      }
   }

   GIVEN( "priority strings in every accepted form" )
   {
      WHEN( "parsing strings back to priorities" )
      {
         REQUIRE( logLevelFromString( "" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "42" ) == 42 );
         REQUIRE( logLevelFromString( "alert" ) == logPrio::LOG_ALERT );
         REQUIRE( logLevelFromString( "crit" ) == logPrio::LOG_CRITICAL );
         REQUIRE( logLevelFromString( "warning" ) == logPrio::LOG_WARNING );
         REQUIRE( logLevelFromString( "notice" ) == logPrio::LOG_NOTICE );
         REQUIRE( logLevelFromString( "info" ) == logPrio::LOG_INFO );
         REQUIRE( logLevelFromString( "E" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "EMERGENCY" ) == logPrio::LOG_EMERGENCY );
         REQUIRE( logLevelFromString( "ERROR" ) == logPrio::LOG_ERROR );
         REQUIRE( logLevelFromString( "EX" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "D" ) == logPrio::LOG_DEBUG );
         REQUIRE( logLevelFromString( "D1" ) == logPrio::LOG_DEBUG );
         REQUIRE( logLevelFromString( "D2" ) == logPrio::LOG_DEBUG2 );
         REQUIRE( logLevelFromString( "DX" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "DBG" ) == logPrio::LOG_DEBUG );
         REQUIRE( logLevelFromString( "DBG2" ) == logPrio::LOG_DEBUG2 );
         REQUIRE( logLevelFromString( "DBGX" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "DEF" ) == logPrio::LOG_DEFAULT );
         REQUIRE( logLevelFromString( "DEBUG" ) == logPrio::LOG_DEBUG );
         REQUIRE( logLevelFromString( "DEBUG2" ) == logPrio::LOG_DEBUG2 );
         REQUIRE( logLevelFromString( "DEBUGX" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "DQRST" ) == logPrio::LOG_UNKNOWN );
         REQUIRE( logLevelFromString( "TELEM" ) == logPrio::LOG_TELEM );
         REQUIRE( logLevelFromString( "zzz" ) == logPrio::LOG_UNKNOWN );
      }
   }
}

// Verifies construction of timespecX from a native timespec, including the clamp of a
// negative time to zero, and the meanTimespecX() average with whole, fractional, and
// carrying second results.
SCENARIO( "timespecX conversions and arithmetic", "[flatlogs::timespecX]" )
{
   GIVEN( "native timespec values" )
   {
      WHEN( "constructing from a normal timespec" )
      {
         timespec ts;
         ts.tv_sec  = 1700000000;
         ts.tv_nsec = 250;

         timespecX tsx( ts );
         REQUIRE( tsx.time_s == 1700000000 );
         REQUIRE( tsx.time_ns == 250 );
      }

      WHEN( "constructing from a negative timespec" )
      {
         timespec ts;
         ts.tv_sec  = -1;
         ts.tv_nsec = 250;

         timespecX tsx( ts );
         REQUIRE( tsx.time_s == 0 );
         REQUIRE( tsx.time_ns == 0 );
      }
   }

   GIVEN( "pairs of timespecX values to average" )
   {
      WHEN( "the mean number of seconds is whole" )
      {
         timespecX a{ 2, 100 }, b{ 4, 300 };
         timespecX m = meanTimespecX( a, b );
         REQUIRE( m.time_s == 3 );
         REQUIRE( m.time_ns == 200 );
      }

      WHEN( "the mean number of seconds is fractional" )
      {
         timespecX a{ 1, 0 }, b{ 2, 0 };
         timespecX m = meanTimespecX( a, b );
         REQUIRE( m.time_s == 1 );
         REQUIRE( m.time_ns == 500000000 );
      }

      WHEN( "the fractional-second adjustment carries into a whole second" )
      {
         timespecX a{ 1, 999999999 }, b{ 2, 999999999 };
         timespecX m = meanTimespecX( a, b );
         REQUIRE( m.time_s == 2 );
         REQUIRE( m.time_ns == 499999999 );
      }
   }
}

} //namespace flatlogs_test
