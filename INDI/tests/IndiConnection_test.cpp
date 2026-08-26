/** \file IndiConnection_test.cpp
  * \brief Catch2 tests for pcf::IndiConnection (INDI/libcommon/IndiConnection.cpp).
  *
  * Drives the real message-processing loop over real pipes.
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

/// Minimal concrete connection: counts dispatches and update() calls.
class TestConnection : public IndiConnection
{
 public:
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

   // process() is private and detachFds() protected in IndiConnection; expose
   // them for direct driving here.
   void runProcess()
   {
      processIndiRequests( false );
   }

   void callDetachFds()
   {
      detachFds();
   }
};

/// update() throws a std::exception subclass, escaping process().
class ThrowingUpdateConnection : public TestConnection
{
 public:
   void update() override
   {
      throw std::runtime_error( "update failed" );
   }
};

/// update() throws a non-std value, escaping process().
class ThrowingIntUpdateConnection : public TestConnection
{
 public:
   void update() override
   {
      throw 42;
   }
};

/// dispatch() throws a runtime_error, caught inside process()'s loop.
class DispatchRuntimeThrowConnection : public TestConnection
{
 public:
   void dispatch( const IndiMessage::Type &, const IndiProperty & ) override
   {
      ++m_dispatches;
      throw std::runtime_error( "dispatch runtime_error" );
   }
};

/// dispatch() throws a logic_error (a std::exception that is NOT a
/// runtime_error), caught by process()'s second handler.
class DispatchLogicThrowConnection : public TestConnection
{
 public:
   void dispatch( const IndiMessage::Type &, const IndiProperty & ) override
   {
      ++m_dispatches;
      throw std::logic_error( "dispatch logic_error" );
   }
};

/// XML for a getProperties request.
static std::string getPropsXml()
{
   IndiProperty ip( IndiProperty::Text );
   ip.setDevice( "dev" );
   ip.setName( "prop" );
   ip.setVersion( "1" );
   IndiXmlParser gen( IndiMessage( IndiMessage::GetProperties, ip ) );
   return gen.createXmlString();
}

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
         ::close( fdIn[1] ); // EOF after the message: read() returns 0 -> quit

         conn.runProcess(); // runs the real loop in this thread until EOF

         REQUIRE( conn.m_dispatches.load() == 1 );
         REQUIRE( conn.m_lastType == IndiMessage::GetProperties );
         REQUIRE( conn.m_lastDevice == "dev" );
         REQUIRE( conn.m_updates.load() > 0 );
         REQUIRE( conn.getQuitProcess() );

         ::close( fdIn[0] );
         ::close( fdOut[0] );
         // fdOut[1] is owned (and closed) by the connection.
      }

      WHEN( "processIndiRequests runs the loop in its own thread" )
      {
         int fdIn[2];
         REQUIRE( ::pipe( fdIn ) == 0 );

         TestConnection conn;
         conn.setInputFd( fdIn[0] );

         conn.activate(); // so deactivate() runs and joins the process thread
         conn.waitForReady();
         conn.processIndiRequests( true );

         std::string xml = getPropsXml();
         REQUIRE( ::write( fdIn[1], xml.c_str(), xml.size() ) == (ssize_t)xml.size() );

         for( int i = 0; i < 200 && conn.m_dispatches.load() == 0; ++i )
         {
            pcf::Thread::msleep( 5 );
         }
         REQUIRE( conn.m_dispatches.load() == 1 );

         conn.quitProcess();
         conn.deactivate(); // joins the process thread

         ::close( fdIn[0] );
         ::close( fdIn[1] );
      }

      WHEN( "activate starts the Thread loop and rejects double activation" )
      {
         TestConnection conn;
         conn.setInterval( 1 ); // so the loop reaches the default execute() quickly
         conn.activate();
         conn.waitForReady(); // the started thread flags itself running
         REQUIRE( conn.isActive() );
         REQUIRE_THROWS_AS( conn.activate(), std::runtime_error );
         pcf::Thread::msleep( 30 ); // let the loop reach the default execute()
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
         pcf::Thread::msleep( 50 ); // the catch prints and the thread exits
         conn.quitProcess();
         REQUIRE( true );
      }

      WHEN( "update() throws a non-std value out of the process thread" )
      {
         ThrowingIntUpdateConnection conn;
         conn.processIndiRequests( true );
         pcf::Thread::msleep( 50 );
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
         conn.runProcess(); // catch(runtime_error) inside the loop, then EOF quits
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
         conn.runProcess();
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
         ::close( fdIn[0] ); // select() on the closed fd genuinely fails with EBADF

         conn.processIndiRequests( true );
         pcf::Thread::msleep( 1200 ); // one full back-off sleep(1)
         conn.quitProcess();
         pcf::Thread::msleep( 1100 ); // let the loop notice and exit
         REQUIRE( true );
      }

      WHEN( "the input fd is a directory, select succeeds but read fails" )
      {
         int fdDir = ::open( ".", O_RDONLY );
         REQUIRE( fdDir >= 0 );

         TestConnection conn;
         conn.setInputFd( fdDir ); // readable per select, but read() fails EISDIR
         conn.runProcess(); // read < 0 -> quit
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
         conn.setOutputFd( fdOut2[1] ); // same fd: early-return branch

         // Writing to a pipe whose read end is closed fails; with SIGPIPE
         // ignored the write loop's error-break path runs for real.
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
         REQUIRE( true );
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

         conn.callDetachFds();
      }
   }
}

} //namespace IndiConnection_test
