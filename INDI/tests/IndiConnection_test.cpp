/** \file IndiConnection_test.cpp
  * \brief Catch2 tests for pcf::IndiConnection in INDI/libcommon/IndiConnection.cpp.
  *
  * The tests drive the real message processing loop over real pipes. No mocks are used.
  * Some tests start the connection's own thread and use short sleeps to let it run.
  * One test closes the input descriptor on purpose so that select() fails with EBADF.
  * That test waits about two seconds to cover the loop's one second back-off.
  * One test ignores SIGPIPE while writing to a pipe with no reader.
  * No ports or privileges are needed.
  */
#include "../../tests/catch2/catch.hpp"

#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>

#include "../libcommon/IndiConnection.hpp"
#include "../libcommon/IndiMessage.hpp"
#include "../libcommon/IndiProperty.hpp"
#include "../libcommon/IndiXmlParser.hpp"

using pcf::IndiConnection;
using pcf::IndiMessage;
using pcf::IndiProperty;
using pcf::IndiXmlParser;

namespace IndiConnection_test
{

/// Minimal concrete connection. It counts dispatch() and update() calls and
/// records the type and device of the last dispatched message.
class TestConnection : public IndiConnection
{
 public:
   /// Uses the name testconn, version 1, and protocol version 1.
   TestConnection() : IndiConnection( "testconn", "1", "1" )
   {
   }

   std::atomic<int> m_dispatches{ 0 };
   std::atomic<int> m_updates{ 0 };

   IndiMessage::Type m_lastType{ IndiMessage::Unknown };
   std::string       m_lastDevice;

   void dispatch( const IndiMessage::Type &tType, const IndiProperty &ipDispatch ) override
   {
      ++m_dispatches;
      m_lastType   = tType;
      m_lastDevice = ipDispatch.getDevice();
   }

   void update() override
   {
      ++m_updates;
   }

   // process() is private and detachFds() is protected in IndiConnection.
   // These wrappers expose them so the tests can drive them directly.
   // runProcess() runs the real loop in the calling thread by passing false
   // to processIndiRequests().
   void runProcess()
   {
      processIndiRequests( false );
   }

   void callDetachFds()
   {
      detachFds();
   }
};

/// update() throws a std::exception subclass. The exception escapes process().
class ThrowingUpdateConnection : public TestConnection
{
 public:
   void update() override
   {
      throw std::runtime_error( "update failed" );
   }
};

/// update() throws a plain int. It is not a std::exception and it escapes process().
class ThrowingIntUpdateConnection : public TestConnection
{
 public:
   void update() override
   {
      throw 42;
   }
};

/// dispatch() throws a std::runtime_error. The loop inside process() catches it
/// and keeps running.
class DispatchRuntimeThrowConnection : public TestConnection
{
 public:
   void dispatch( const IndiMessage::Type &, const IndiProperty & ) override
   {
      ++m_dispatches;
      throw std::runtime_error( "dispatch runtime_error" );
   }
};

/// dispatch() throws a std::logic_error. That is a std::exception but not a
/// runtime_error, so the second handler in process() catches it.
class DispatchLogicThrowConnection : public TestConnection
{
 public:
   void dispatch( const IndiMessage::Type &, const IndiProperty & ) override
   {
      ++m_dispatches;
      throw std::logic_error( "dispatch logic_error" );
   }
};

/// Builds the XML for a getProperties request for device dev and property prop.
static std::string getPropsXml()
{
   IndiProperty ip( IndiProperty::Text );
   ip.setDevice( "dev" );
   ip.setName( "prop" );
   ip.setVersion( "1" );
   IndiXmlParser gen( IndiMessage( IndiMessage::GetProperties, ip ) );
   return gen.createXmlString();
}

// Verifies the whole connection with real descriptors. It covers message processing
// over pipes, the background thread, exception handling in the overrides, the output
// side, and the small accessors.
SCENARIO( "IndiConnection processes real INDI traffic over pipes", "[IndiConnection]" )
{
   GIVEN( "a connection wired to pipes" )
   {
      WHEN( "process() dispatches a message and quits on EOF" )
      {
         int fdIn[2], fdOut[2];
         REQUIRE( ::pipe( fdIn ) == 0 );
         REQUIRE( ::pipe( fdOut ) == 0 );

         TestConnection conn;
         conn.setInputFd( fdIn[0] );
         conn.setOutputFd( fdOut[1] );

         std::string xml = getPropsXml();
         REQUIRE( ::write( fdIn[1], xml.c_str(), xml.size() ) == (ssize_t)xml.size() );
         ::close( fdIn[1] ); // Close the write end so read() sees EOF after the message. EOF makes the loop quit.

         conn.runProcess(); // Run the real loop in this thread until EOF.

         REQUIRE( conn.m_dispatches.load() == 1 );
         REQUIRE( conn.m_lastType == IndiMessage::GetProperties );
         REQUIRE( conn.m_lastDevice == "dev" );
         REQUIRE( conn.m_updates.load() > 0 );
         REQUIRE( conn.getQuitProcess() );

         ::close( fdIn[0] );
         ::close( fdOut[0] );
         // fdOut[1] is left with the connection. setOutputFd() closes it if the fd is ever replaced.
      }

      WHEN( "processIndiRequests runs the loop in its own thread" )
      {
         int fdIn[2];
         REQUIRE( ::pipe( fdIn ) == 0 );

         TestConnection conn;
         conn.setInputFd( fdIn[0] );

         conn.activate(); // Activate first so that deactivate() later joins the process thread.
         conn.waitForReady();
         conn.processIndiRequests( true );

         std::string xml = getPropsXml();
         REQUIRE( ::write( fdIn[1], xml.c_str(), xml.size() ) == (ssize_t)xml.size() );

         // Wait up to one second for the process thread to dispatch the message.
         for( int i = 0; i < 200 && conn.m_dispatches.load() == 0; ++i )
         {
            pcf::Thread::msleep( 5 );
         }
         REQUIRE( conn.m_dispatches.load() == 1 );

         conn.quitProcess();
         conn.deactivate(); // deactivate() joins the process thread.

         ::close( fdIn[0] );
         ::close( fdIn[1] );
      }

      WHEN( "activate starts the Thread loop and rejects double activation" )
      {
         TestConnection conn;
         conn.setInterval( 1 ); // A one millisecond interval lets the loop reach the default execute() quickly.
         conn.activate();
         conn.waitForReady(); // This returns once the started thread flags itself running.
         REQUIRE( conn.isActive() );
         REQUIRE_THROWS_AS( conn.activate(), std::runtime_error );
         pcf::Thread::msleep( 30 ); // Let the loop reach the default execute() at least once.
         conn.deactivate();
         REQUIRE( !conn.isActive() );
      }
   }

   GIVEN( "connections whose overrides throw" )
   {
      WHEN( "update() throws a std::exception out of the process thread" )
      {
         ThrowingUpdateConnection conn;
         conn.processIndiRequests( true );
         pcf::Thread::msleep( 50 ); // The catch handler prints the error and the thread exits. Give it time.
         conn.quitProcess();
         REQUIRE( true ); // Reaching this point without a crash is the check.
      }

      WHEN( "update() throws a non-std value out of the process thread" )
      {
         ThrowingIntUpdateConnection conn;
         conn.processIndiRequests( true );
         pcf::Thread::msleep( 50 ); // Same as above. The catch-all handler runs and the thread exits.
         conn.quitProcess();
         REQUIRE( true );
      }

      WHEN( "dispatch() throws inside the processing loop" )
      {
         int fdIn[2];
         REQUIRE( ::pipe( fdIn ) == 0 );

         DispatchRuntimeThrowConnection conn;
         conn.setInputFd( fdIn[0] );
         std::string xml = getPropsXml();
         REQUIRE( ::write( fdIn[1], xml.c_str(), xml.size() ) == (ssize_t)xml.size() );
         ::close( fdIn[1] );
         conn.runProcess(); // The runtime_error handler inside the loop catches the throw. EOF then quits.
         REQUIRE( conn.m_dispatches.load() == 1 );
         ::close( fdIn[0] );
      }

      WHEN( "dispatch() throws a non-runtime std exception inside the loop" )
      {
         int fdIn[2];
         REQUIRE( ::pipe( fdIn ) == 0 );

         DispatchLogicThrowConnection conn;
         conn.setInputFd( fdIn[0] );
         std::string xml = getPropsXml();
         REQUIRE( ::write( fdIn[1], xml.c_str(), xml.size() ) == (ssize_t)xml.size() );
         ::close( fdIn[1] );
         conn.runProcess(); // The std::exception handler inside the loop catches the throw. EOF then quits.
         REQUIRE( conn.m_dispatches.load() == 1 );
         ::close( fdIn[0] );
      }

      WHEN( "the input fd is closed, select fails and the loop backs off" )
      {
         int fdIn[2];
         REQUIRE( ::pipe( fdIn ) == 0 );
         ::close( fdIn[1] );

         TestConnection conn;
         conn.setInputFd( fdIn[0] );
         ::close( fdIn[0] ); // Close the fd after handing it over so select() on it fails with EBADF.

         conn.processIndiRequests( true );
         pcf::Thread::msleep( 1200 ); // Wait through one full back-off sleep of one second.
         conn.quitProcess();
         pcf::Thread::msleep( 1100 ); // Give the loop time to notice quitProcess() and exit.
         REQUIRE( true ); // Surviving the back-off without a crash is the check.
      }

      WHEN( "the input fd is a directory, select succeeds but read fails" )
      {
         int fdDir = ::open( ".", O_RDONLY );
         REQUIRE( fdDir >= 0 );

         TestConnection conn;
         conn.setInputFd( fdDir ); // A directory is readable according to select(), but read() fails with EISDIR.
         conn.runProcess(); // A negative read result makes the loop quit.
         REQUIRE( conn.getQuitProcess() );

         ::close( fdDir );
      }
   }

   GIVEN( "the output side" )
   {
      WHEN( "sendXml writes to a real pipe" )
      {
         int fdOut[2];
         REQUIRE( ::pipe( fdOut ) == 0 );

         TestConnection conn;
         conn.setOutputFd( fdOut[1] );
         conn.sendXml( "<ping/>" );

         char buf[16] = { 0 };
         REQUIRE( ::read( fdOut[0], buf, sizeof( buf ) - 1 ) == 7 );
         REQUIRE( std::string( buf ) == "<ping/>" );

         // Replacing the output fd closes the previous one.
         int fdOut2[2];
         REQUIRE( ::pipe( fdOut2 ) == 0 );
         conn.setOutputFd( fdOut2[1] );
         conn.setOutputFd( fdOut2[1] ); // Setting the same fd again takes the early return branch.

         // Writing to a pipe whose read end is closed fails. SIGPIPE is ignored so
         // the process survives and the write loop's error break path runs for real.
         ::close( fdOut2[0] );
         void ( *prev )( int ) = ::signal( SIGPIPE, SIG_IGN );
         conn.sendXml( "<pong/>" );
         ::signal( SIGPIPE, prev );

         ::close( fdOut[0] );
      }

      WHEN( "sendXml without an output fd returns immediately" )
      {
         TestConnection conn;
         conn.setOutputFd( -1 );
         conn.sendXml( "<nothing/>" );
         REQUIRE( true ); // The call must return without touching any descriptor.
      }
   }

   GIVEN( "the small accessors" )
   {
      WHEN( "reading and setting them" )
      {
         TestConnection conn;
         REQUIRE( conn.getName() == "testconn" );
         REQUIRE( conn.getVersion() == "1" );
         REQUIRE( conn.getProtocolVersion() == "1" );
         conn.setVersion( "2" );
         REQUIRE( conn.getVersion() == "2" );
         conn.setName( "renamed" );
         REQUIRE( conn.getName() == "renamed" );
         conn.setProtocolVersion( "1.7" );
         REQUIRE( conn.getProtocolVersion() == "1.7" );

         REQUIRE( !conn.isVerboseModeEnabled() );
         conn.enableVerboseMode( true );
         REQUIRE( conn.isVerboseModeEnabled() );

         conn.callDetachFds(); // Resets both descriptors to -1 without closing anything.
      }
   }
}

} //namespace IndiConnection_test
