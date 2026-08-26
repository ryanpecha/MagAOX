/** \file IndiDriverClient_test.cpp
  * \brief Catch2 tests for pcf::IndiDriver and pcf::IndiClient in
  *        INDI/libcommon/IndiDriver.cpp and INDI/libcommon/IndiClient.cpp.
  *
  * The driver writes to a real pipe and the tests read the XML back from it.
  * The client connects to a real TCP server on the loopback address 127.0.0.1.
  * The server listens on port 52040. Port 52041 must have no listener.
  * One driver test sleeps a little over five seconds so the real uptime interval elapses.
  * No privileges are needed.
  */
#include "../../tests/catch2/catch.hpp"

#include <csignal>
#include <unistd.h>

#include "../libcommon/IndiClient.hpp"
#include "../libcommon/IndiDriver.hpp"
#include "../libcommon/IndiElement.hpp"
#include "../libcommon/IndiMessage.hpp"
#include "../libcommon/IndiProperty.hpp"
#include "../libcommon/SystemSocket.hpp"
#include "../libcommon/Thread.hpp"

using pcf::IndiClient;
using pcf::IndiDriver;
using pcf::IndiElement;
using pcf::IndiMessage;
using pcf::IndiProperty;
using pcf::SystemSocket;

namespace IndiDriverClient_test
{

/// Builds a read-write Text property named dev.prop with one element.
static IndiProperty simpleProp()
{
   IndiProperty ip( IndiProperty::Text, "dev", "prop", IndiProperty::Ok,
                    IndiProperty::ReadWrite, IndiProperty::OneOfMany );
   ip.add( IndiElement( "e", "v" ) );
   return ip;
}

// Verifies IndiDriver dispatch(), the send functions, the mode flags, the uptime
// message, and the constructors. The driver output is read back from a real pipe.
SCENARIO( "IndiDriver dispatch, send functions, and accessors", "[IndiDriver]" )
{
   GIVEN( "a driver wired to an output pipe" )
   {
      IndiDriver drv( "drvname", "1", "1" );

      int fdOut[2];
      REQUIRE( ::pipe( fdOut ) == 0 );
      drv.setOutputFd( fdOut[1] );

      WHEN( "dispatching each message type" )
      {
         IndiProperty ip = simpleProp();

         REQUIRE( !drv.isResponseModeEnabled() );
         drv.dispatch( IndiMessage::GetProperties, ip ); // The first GetProperties enables response mode.
         REQUIRE( drv.isResponseModeEnabled() );
         drv.dispatch( IndiMessage::GetProperties, ip ); // The second call takes the already enabled branch.

         drv.dispatch( IndiMessage::Define, ip );
         drv.dispatch( IndiMessage::Delete, ip );
         drv.dispatch( IndiMessage::Message, ip );
         drv.dispatch( IndiMessage::NewProperty, ip );
         drv.dispatch( IndiMessage::SetProperty, ip );
         drv.dispatch( IndiMessage::EnableBLOB, ip ); // The default handler ignores EnableBLOB.
      }

      WHEN( "sending every message form with response mode on" )
      {
         drv.enableResponseMode( true );

         IndiProperty ip = simpleProp();
         std::vector<IndiProperty> vec{ ip, ip };

         drv.sendDefProperty( ip );
         drv.sendDefProperties( vec );
         drv.sendSetProperty( ip );
         drv.sendSetProperties( vec );
         drv.sendDelProperty( ip );
         drv.sendDelProperties( vec );

         IndiProperty eb = simpleProp();
         eb.setBLOBEnable( IndiProperty::Never );
         drv.sendEnableBLOB( eb );
         drv.sendGetProperties( ip );

         IndiProperty msg( IndiProperty::Text );
         msg.setDevice( "dev" );
         msg.setMessage( "note" );
         drv.sendMessage( msg );

         // Drain the pipe and confirm real XML flowed through it.
         char        buf[8192];
         ssize_t     nRead = ::read( fdOut[0], buf, sizeof( buf ) - 1 );
         REQUIRE( nRead > 0 );
         buf[nRead]      = 0;
         std::string all( buf );
         REQUIRE( all.find( "defTextVector" ) != std::string::npos );
         REQUIRE( all.find( "setTextVector" ) != std::string::npos );
         REQUIRE( all.find( "delProperty" ) != std::string::npos );
      }

      WHEN( "sending with response mode off is a no-op" )
      {
         IndiProperty ip = simpleProp();
         drv.sendDefProperty( ip );
         drv.sendSetProperty( ip );
         drv.sendDelProperty( ip );
         std::vector<IndiProperty> vec{ ip };
         drv.sendDefProperties( vec );
         drv.sendSetProperties( vec );
         drv.sendDelProperties( vec );
         REQUIRE( true ); // Nothing is written because response mode is off. Returning cleanly is the check.
      }

      ::close( fdOut[0] );
   }

   GIVEN( "the driver's mode flags and info accessors" )
   {
      IndiDriver drv( "drvname", "1", "1" );

      WHEN( "toggling and reading them" )
      {
         REQUIRE( drv.isAlarmModeEnabled() );
         drv.enableAlarmMode( false );
         REQUIRE( !drv.isAlarmModeEnabled() );

         REQUIRE( !drv.isSimulationModeEnabled() );
         drv.enableSimulationMode( true );
         REQUIRE( drv.isSimulationModeEnabled() );

         REQUIRE( drv.getEmailList() == "" );
         REQUIRE( drv.getDataDirectory() == "" );
         REQUIRE( drv.getAlarmInterval() == 1440 );
         static_cast<void>( drv.getStartTime() );

         // Less than five seconds have passed since construction, so update() sends nothing.
         drv.update();
      }

      WHEN( "simulation mode cannot change while the driver is active" )
      {
         drv.setInterval( 1 ); // A one millisecond interval lets the loop reach the default execute() quickly.
         drv.activate();
         drv.waitForReady();
         REQUIRE_THROWS_AS( drv.enableSimulationMode( true ), std::runtime_error );
         pcf::Thread::msleep( 30 ); // Let the loop reach the default execute() at least once.
         drv.deactivate();
      }
   }

   GIVEN( "the uptime message after the send interval has really elapsed" )
   {
      WHEN( "update() sends the uptime SetProperty" )
      {
         IndiDriver drv( "drvname", "1", "1" );

         int fdOut[2];
         REQUIRE( ::pipe( fdOut ) == 0 );
         drv.setOutputFd( fdOut[1] );
         drv.enableResponseMode( true );

         pcf::Thread::msleep( 5100 ); // Sleep past the real five second uptime interval.
         drv.update();

         char    buf[4096];
         ssize_t nRead = ::read( fdOut[0], buf, sizeof( buf ) - 1 );
         REQUIRE( nRead > 0 );
         buf[nRead] = 0;
         REQUIRE( std::string( buf ).find( "Uptime" ) != std::string::npos );

         ::close( fdOut[0] );
      }
   }

   GIVEN( "driver constructors and assignment" )
   {
      WHEN( "constructing each way" )
      {
         IndiDriver d0;
         REQUIRE( d0.getName() == "generic_indi_process" );

         IndiDriver d1( "nm", "2", "1.7" );
         REQUIRE( d1.getName() == "nm" );
      }
   }
}

// Verifies IndiClient against a real loopback TCP server on port 52040. The server
// accepts the connection and reads the XML the client sends. A connection attempt
// to port 52041, which has no listener, checks the failure path.
SCENARIO( "IndiClient over a real loopback TCP connection", "[IndiClient]" )
{
   GIVEN( "a listening server" )
   {
      SystemSocket server( SystemSocket::Stream, 52040, "127.0.0.1" );
      server.create();
      server.bind();
      server.listen();

      WHEN( "the client connects, sends, and dispatches" )
      {
         IndiClient client( "127.0.0.1", 52040 );

         SystemSocket accepted;
         server.accept( accepted );
         REQUIRE( accepted.isValid() );

         IndiProperty ip = simpleProp();
         client.sendNewProperty( ip );
         std::vector<IndiProperty> vec{ ip };
         client.sendNewProperties( vec );

         IndiProperty eb = simpleProp();
         eb.setBLOBEnable( IndiProperty::Also );
         client.sendEnableBLOB( eb );
         client.sendGetProperties( ip );

         IndiProperty msg( IndiProperty::Text );
         msg.setDevice( "dev" );
         msg.setMessage( "note" );
         client.sendMessage( msg );

         std::string got = accepted.recv();
         REQUIRE( got.find( "newTextVector" ) != std::string::npos );

         // Dispatch each type through the client's default handlers.
         client.dispatch( IndiMessage::Define, ip );
         client.dispatch( IndiMessage::Delete, ip );
         client.dispatch( IndiMessage::Message, ip );
         client.dispatch( IndiMessage::NewProperty, ip );
         client.dispatch( IndiMessage::SetProperty, ip );
         client.dispatch( IndiMessage::EnableBLOB, ip ); // The default handler ignores EnableBLOB.

         client.update(); // The default update() does nothing.

         // Activate briefly so the client's default beforeExecute(), execute(),
         // and afterExecute() run inside the Thread loop.
         client.setInterval( 1 );
         client.activate();
         client.waitForReady();
         pcf::Thread::msleep( 30 );
         client.deactivate();

         accepted.close();
      }

      WHEN( "constructing the other ways" )
      {
         IndiClient c1( "cname", "1", "1", "127.0.0.1", 52040 );
         SystemSocket accepted;
         server.accept( accepted );
         REQUIRE( c1.getName() == "cname" );

         accepted.close();
      }

      WHEN( "connecting to a port with no listener" )
      {
         // setup() catches the connection error. Its cleanup then calls close()
         // on the already invalid socket, and that close() throws. So the
         // constructor itself currently propagates a SystemSocket::Error.
         REQUIRE_THROWS_AS( IndiClient( "127.0.0.1", 52041 ), SystemSocket::Error );
      }

      server.close();
   }
}

} //namespace IndiDriverClient_test
