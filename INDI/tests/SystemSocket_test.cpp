/** \file SystemSocket_test.cpp
  * \brief Catch2 tests for pcf::SystemSocket in INDI/libcommon/SystemSocket.cpp.
  *
  * No mocks are used. Every path is exercised with real sockets on the loopback
  * interface. This includes TCP client and server pairs, UDP datagrams, and real
  * error conditions such as refused connections, closed peers, invalid descriptors,
  * and an exhausted file descriptor limit set with setrlimit(). The tests bind fixed
  * loopback ports in the 52000 range, so they must not run in parallel with each other.
  */
#include "../../tests/catch2/catch.hpp"

#include <cerrno>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include "../libcommon/SystemSocket.hpp"

using pcf::SystemSocket;

namespace SystemSocket_test
{

// Verifies the constructors, copy semantics, and the static address and interface helpers.
// Copies must not share the file descriptor, and assignment must close any descriptor the
// target already owns.
SCENARIO( "SystemSocket construction, copying, and static helpers", "[SystemSocket]" )
{
   GIVEN( "constructors and copies" )
   {
      WHEN( "constructing, copying, and assigning" )
      {
         SystemSocket s0;
         REQUIRE( s0.getType() == SystemSocket::UnknownType );
         REQUIRE( !s0.isValid() );
         REQUIRE( !s0.isBound() );
         REQUIRE( s0.getLastError() == 0 );
         REQUIRE( s0.getFd() == -1 );

         SystemSocket s1( SystemSocket::Stream, 52001, "127.0.0.1" );
         REQUIRE( s1.getType() == SystemSocket::Stream );
         REQUIRE( s1.getPort() == 52001 );
         REQUIRE( s1.getHost() == "127.0.0.1" );
         s1.setConnectTimeout( 250 );
         s1.create();
         REQUIRE( s1.isValid() );

         SystemSocket s2( s1 ); // fd is deliberately not copied
         REQUIRE( s2.getPort() == 52001 );
         REQUIRE( !s2.isValid() );

         SystemSocket s3;
         s3.create(); // UnknownType: stays invalid
         REQUIRE( !s3.isValid() );

         SystemSocket s4( SystemSocket::Datagram, 52002, "127.0.0.1" );
         s4.create(); // valid fd, so operator= must close it
         REQUIRE( s4.isValid() );
         s4 = s1;
         REQUIRE( s4.getPort() == 52001 );
         REQUIRE( !s4.isValid() );
         s4 = s4; // self-assignment branch
         REQUIRE( s4.getPort() == 52001 );

         s1.close();
      }
   }

   GIVEN( "the address helpers" )
   {
      WHEN( "converting host strings to addresses" )
      {
         sockaddr_in any = SystemSocket::convertStrToAddr( "" );
         REQUIRE( any.sin_addr.s_addr == htonl( INADDR_ANY ) );

         sockaddr_in lo = SystemSocket::convertStrToAddr( "127.0.0.1" );
         REQUIRE( lo.sin_addr.s_addr == inet_addr( "127.0.0.1" ) );

         REQUIRE_THROWS_AS( SystemSocket::convertStrToAddr( "not-an-ip" ), SystemSocket::Error );

         sockaddr_in sa = SystemSocket::createSockAddr( 52003, "127.0.0.1" );
         REQUIRE( sa.sin_port == htons( 52003 ) );
      }
   }

   GIVEN( "host and interface utilities" )
   {
      WHEN( "querying the local host and interfaces" )
      {
         REQUIRE( SystemSocket::getLocalHostName().size() > 0 );

         std::vector<SystemSocket::Interface> ifaces;
         int n = SystemSocket::getInterfaces( ifaces );
         REQUIRE( n >= 1 ); // at least loopback

         SystemSocket::Interface lo = SystemSocket::getInterface( "lo" );
         REQUIRE( lo.getIp().size() > 0 );
         static_cast<void>( lo.getBroadcast() );
         static_cast<void>( lo.getName() );

         // Unknown name falls back to the loopback default.
         SystemSocket::Interface dflt = SystemSocket::getInterface( "no-such-iface" );
         REQUIRE( dflt.getIp() == "127.0.0.1" );
      }
   }
}

// Verifies a full TCP round trip on loopback. A real server listens, a real client connects,
// and data is exchanged with both the string and chunk interfaces. Socket options and a peer
// that closes in the middle of a chunk read are also checked.
SCENARIO( "SystemSocket TCP end to end on loopback", "[SystemSocket]" )
{
   GIVEN( "a listening server and a client" )
   {
      SystemSocket server( SystemSocket::Stream, 52010, "127.0.0.1" );
      server.create();
      server.bind();
      server.listen();

      SystemSocket client( SystemSocket::Stream, 52010, "127.0.0.1" );
      client.setConnectTimeout( 2000 );
      client.connect();

      SystemSocket accepted;
      server.accept( accepted );
      REQUIRE( accepted.isValid() );

      WHEN( "exchanging data with send/recv and the chunk APIs" )
      {
         client.send( "hello" );
         std::string got = accepted.recv();
         REQUIRE( got == "hello" );

         accepted.send( "world" );
         REQUIRE( client.recv() == "world" );

         char chunkOut[8] = "chunk67";
         int  n = 7;
         client.sendChunk( chunkOut, n );
         REQUIRE( n == 7 );

         char chunkIn[8];
         n = 7;
         accepted.recvChunk( chunkIn, n );
         REQUIRE( n == 7 );
         REQUIRE( std::string( chunkIn, 7 ) == "chunk67" );

         accepted.send( "more" );
         REQUIRE( client.recv() == "more" );
      }

      WHEN( "toggling socket options" )
      {
         REQUIRE( !client.isNagleDisabled() );
         client.disableNagle( true );
         client.disableNagle( false );

         client.setRecvTimeout( 50 );
         REQUIRE_THROWS_AS( client.recv(), SystemSocket::Error ); // real timeout

         client.setNonBlocking( true );
         client.setNonBlocking( false );

         int nVal = 0;
         unsigned int uiLen = sizeof( nVal );
         client.getOption( SOL_SOCKET, SO_REUSEADDR, &nVal, uiLen );

         int nTrue = 1;
         client.setOption( SOL_SOCKET, SO_KEEPALIVE, &nTrue, sizeof( nTrue ) );
      }

      WHEN( "the peer closes mid-recvChunk" )
      {
         char part[4] = "abc";
         int  n = 3;
         accepted.sendChunk( part, n );
         accepted.close();

         // Ask for more than was sent. After the 3 bytes, recv() returns 0 because the
         // peer shut down, and the wrapper throws ECONNRESET.
         char buf[10];
         n = 10;
         REQUIRE_THROWS_AS( client.recvChunk( buf, n ), SystemSocket::Error );
      }

      client.close();
      if( accepted.isValid() )
      {
         accepted.close();
      }
      server.close();
   }
}

// Verifies datagram send and receive on loopback with both the string and chunk interfaces.
// A receiver of the multicast type is also fed plain loopback datagrams so that its distinct
// receive branches run for real.
SCENARIO( "SystemSocket UDP and multicast-type paths on loopback", "[SystemSocket]" )
{
   GIVEN( "a bound datagram receiver and a sender" )
   {
      SystemSocket rx( SystemSocket::Datagram, 52020, "127.0.0.1" );
      rx.create();
      rx.bind();

      SystemSocket tx( SystemSocket::Datagram, 52020, "127.0.0.1" );
      tx.create();

      WHEN( "sending and receiving datagrams" )
      {
         tx.sendTo( "dgram" );
         REQUIRE( rx.recvFrom() == "dgram" );

         char out[6] = "chnk5";
         int  n = 5;
         tx.sendChunkTo( out, n );
         REQUIRE( n == 5 );

         char in[6];
         n = 5;
         rx.recvChunkFrom( in, n );
         REQUIRE( n == 5 );
         REQUIRE( std::string( in, 5 ) == "chnk5" );
      }

      tx.close();
      rx.close();
   }

   GIVEN( "a multicast-type receiver bound to INADDR_ANY" )
   {
      // The multicast type only changes socket options and the receive-side address
      // handling. A plain loopback datagram still reaches it. That exercises the multicast
      // branches of recvFrom() and recvChunkFrom() for real.
      SystemSocket mrx( SystemSocket::enumMulticast, 52021, "" );
      mrx.create();
      mrx.bind();

      SystemSocket tx( SystemSocket::Datagram, 52021, "127.0.0.1" );
      tx.create();

      WHEN( "receiving on the multicast-type socket" )
      {
         tx.sendTo( "mc1" );
         REQUIRE( mrx.recvFrom() == "mc1" );

         tx.sendTo( "mc2" );
         char in[4];
         int  n = 3;
         mrx.recvChunkFrom( in, n );
         REQUIRE( std::string( in, 3 ) == "mc2" );
      }

      WHEN( "joining a real multicast group" )
      {
         SystemSocket mc( SystemSocket::enumMulticast, 52022, "239.255.0.1" );
         mc.create();
         try
         {
            mc.join(); // This needs a multicast-capable interface. Either outcome is acceptable.
         }
         catch( const SystemSocket::Error & )
         {
         }
         mc.close();
      }

      tx.close();
      mrx.close();
   }
}

// Verifies that operations on a never-created socket throw, and that the operating system
// rejections for a duplicate bind, listening on a datagram socket, accepting without
// listening, a refused connection, and an unroutable connect all surface as exceptions.
SCENARIO( "SystemSocket real error paths", "[SystemSocket]" )
{
   GIVEN( "an invalid (never-created) socket" )
   {
      SystemSocket bad;

      WHEN( "every operation that requires a descriptor throws EBADF" )
      {
         char buf[4];
         int  n = 4;
         REQUIRE_THROWS_AS( bad.sendChunk( buf, n ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.recvChunk( buf, n ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.send( "x" ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.recv(), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.setRecvTimeout( 10 ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.setNonBlocking( true ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.disableNagle( true ), SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.close(), SystemSocket::Error );

         int nVal = 0;
         unsigned int uiLen = sizeof( nVal );
         REQUIRE_THROWS_AS( bad.getOption( SOL_SOCKET, SO_REUSEADDR, &nVal, uiLen ),
                            SystemSocket::Error );
         REQUIRE_THROWS_AS( bad.setOption( SOL_SOCKET, SO_REUSEADDR, &nVal, sizeof( nVal ) ),
                            SystemSocket::Error );
      }
   }

   GIVEN( "operations that the OS genuinely rejects" )
   {
      WHEN( "binding two sockets to the same port" )
      {
         // SO_REUSEADDR permits rebinding TIME_WAIT addresses but not two
         // concurrent listeners, so the second bind genuinely fails.
         SystemSocket a( SystemSocket::Stream, 52030, "127.0.0.1" );
         a.bind();
         a.listen();
         SystemSocket b( SystemSocket::Stream, 52030, "127.0.0.1" );
         REQUIRE_THROWS_AS( ( b.bind(), b.listen() ), SystemSocket::Error );
         b.close();
         a.close();
      }

      WHEN( "listening on a datagram socket" )
      {
         SystemSocket s( SystemSocket::Datagram, 52031, "127.0.0.1" );
         s.create();
         s.bind();
         REQUIRE_THROWS_AS( s.listen(), SystemSocket::Error );
         s.close();
      }

      WHEN( "accepting on a socket that is not listening" )
      {
         SystemSocket s( SystemSocket::Stream, 52032, "127.0.0.1" );
         s.bind();
         SystemSocket newSock;
         REQUIRE_THROWS_AS( s.accept( newSock ), SystemSocket::Error );
         s.close();
      }

      WHEN( "connecting to a loopback port with no listener" )
      {
         SystemSocket s( SystemSocket::Stream, 52033, "127.0.0.1" );
         s.setConnectTimeout( 1000 );
         REQUIRE_THROWS_AS( s.connect(), SystemSocket::Error );
         if( s.isValid() )
         {
            s.close();
         }
      }

      WHEN( "connecting to a blackhole address times out" )
      {
         // 10.255.255.1 is not routed here, so the non-blocking connect stays pending and
         // the select() times out. On some hosts the network stack reports the address as
         // unreachable instead. Both are real failures and both must throw.
         SystemSocket s( SystemSocket::Stream, 80, "10.255.255.1" );
         s.setConnectTimeout( 300 );
         REQUIRE_THROWS_AS( s.connect(), SystemSocket::Error );
         if( s.isValid() )
         {
            s.close();
         }
      }
   }
}

// Verifies the remaining fault paths and the entry points that create or bind a socket on
// demand. The socket() failures are produced by lowering RLIMIT_NOFILE for real. The stale
// descriptor, zero-length datagram, port 0, closed peer, and bad option cases are all real
// kernel rejections.
SCENARIO( "SystemSocket remaining real fault and auto-create paths", "[SystemSocket]" )
{
   GIVEN( "sockets left to their destructors and re-created" )
   {
      WHEN( "a valid socket is destroyed without an explicit close" )
      {
         {
            SystemSocket s( SystemSocket::Datagram, 52050, "127.0.0.1" );
            s.create();
            REQUIRE( s.isValid() );
         } // The destructor closes it here.
         REQUIRE( true );
      }

      WHEN( "close() fails because the fd was already closed underneath" )
      {
         SystemSocket s( SystemSocket::Datagram, 52062, "127.0.0.1" );
         s.create();
         ::close( s.getFd() ); // The descriptor held by the object is now stale.
         REQUIRE_THROWS_AS( s.close(), SystemSocket::Error ); // ::close() fails with EBADF.

         // Revive the descriptor number so the close() in the destructor succeeds instead
         // of terminating the process.
         int dummy = ::open( "/dev/null", O_RDONLY );
         REQUIRE( dummy >= 0 );
         REQUIRE( ::dup2( dummy, s.getFd() ) == s.getFd() );
         if( dummy != s.getFd() )
         {
            ::close( dummy );
         }
      }

      WHEN( "create() on an already-valid socket closes and re-creates" )
      {
         SystemSocket s( SystemSocket::Datagram, 52051, "127.0.0.1" );
         s.create();
         int fd1 = s.getFd();
         s.create();
         REQUIRE( s.isValid() );
         static_cast<void>( fd1 );
         s.close();
      }
   }

   GIVEN( "a genuinely exhausted file-descriptor limit" )
   {
      WHEN( "socket() fails for every socket type" )
      {
         struct rlimit old;
         REQUIRE( getrlimit( RLIMIT_NOFILE, &old ) == 0 );
         struct rlimit low = old;
         low.rlim_cur = 3;
         REQUIRE( setrlimit( RLIMIT_NOFILE, &low ) == 0 );

         SystemSocket st( SystemSocket::Stream, 52052, "127.0.0.1" );
         REQUIRE_THROWS_AS( st.create(), SystemSocket::Error );
         SystemSocket sd( SystemSocket::Datagram, 52052, "127.0.0.1" );
         REQUIRE_THROWS_AS( sd.create(), SystemSocket::Error );
         SystemSocket sm( SystemSocket::enumMulticast, 52052, "127.0.0.1" );
         REQUIRE_THROWS_AS( sm.create(), SystemSocket::Error );

         // The internal socket() call inside getInterfaces() also fails.
         std::vector<SystemSocket::Interface> ifaces;
         REQUIRE( SystemSocket::getInterfaces( ifaces ) == -1 );

         // recvFrom() and recvChunkFrom() on an invalid socket reach their internal
         // create() calls, which fail here. No blocking recvfrom() is needed.
         SystemSocket sr( SystemSocket::Datagram, 52061, "127.0.0.1" );
         REQUIRE_THROWS_AS( sr.recvFrom(), SystemSocket::Error );
         char b[4];
         int  n = 4;
         REQUIRE_THROWS_AS( sr.recvChunkFrom( b, n ), SystemSocket::Error );

         REQUIRE( setrlimit( RLIMIT_NOFILE, &old ) == 0 );
      }
   }

   GIVEN( "the auto-create/auto-bind entry points" )
   {
      WHEN( "listen() on a fresh socket binds it first" )
      {
         SystemSocket s( SystemSocket::Stream, 52053, "127.0.0.1" );
         s.listen(); // Not bound yet, so listen() calls bind(), which calls create() first.
         REQUIRE( s.isBound() );
         s.close();
      }

      WHEN( "accept() on an unbound socket binds first, then fails (not listening)" )
      {
         SystemSocket s( SystemSocket::Stream, 52054, "127.0.0.1" );
         SystemSocket n;
         REQUIRE_THROWS_AS( s.accept( n ), SystemSocket::Error );
         s.close();
      }

      WHEN( "the sendTo/recvFrom family auto-creates fresh sockets" )
      {
         SystemSocket rx( SystemSocket::Datagram, 52055, "127.0.0.1" );
         rx.bind(); // This also creates the socket.

         SystemSocket tx( SystemSocket::Datagram, 52055, "127.0.0.1" );
         tx.sendTo( "auto1" ); // This creates the socket.
         REQUIRE( rx.recvFrom() == "auto1" );

         SystemSocket tx2( SystemSocket::Datagram, 52055, "127.0.0.1" );
         char out[6] = "auto2";
         int  n = 5;
         tx2.sendChunkTo( out, n ); // This creates the socket.
         char in[6];
         n = 5;
         rx.recvChunkFrom( in, n );
         REQUIRE( std::string( in, 5 ) == "auto2" );

         // Fresh sockets with a short receive timeout, so recvfrom() genuinely times out
         // and the error branches of recvFrom() and recvChunkFrom() run.
         SystemSocket rx2( SystemSocket::Datagram, 52056, "127.0.0.1" );
         rx2.create();
         rx2.bind();
         rx2.setRecvTimeout( 50 );
         REQUIRE_THROWS_AS( rx2.recvFrom(), SystemSocket::Error );
         char b[4];
         n = 4;
         REQUIRE_THROWS_AS( rx2.recvChunkFrom( b, n ), SystemSocket::Error );

         tx.close();
         tx2.close();
         rx.close();
         rx2.close();
      }
   }

   GIVEN( "datagram edge cases" )
   {
      WHEN( "a zero-length datagram reads as a peer shutdown" )
      {
         SystemSocket rx( SystemSocket::Datagram, 52057, "127.0.0.1" );
         rx.bind();

         SystemSocket tx( SystemSocket::Datagram, 52057, "127.0.0.1" );
         tx.create();
         tx.sendTo( "" ); // A real zero-length datagram.

         REQUIRE_THROWS_AS( rx.recvFrom(), SystemSocket::Error ); // A zero return is reported as ECONNRESET.

         tx.sendTo( "" );
         char b[4];
         int  n = 4;
         REQUIRE_THROWS_AS( rx.recvChunkFrom( b, n ), SystemSocket::Error );

         tx.close();
         rx.close();
      }

      WHEN( "sending to port 0 fails in the kernel" )
      {
         SystemSocket tx( SystemSocket::Datagram, 0, "127.0.0.1" );
         tx.create();
         REQUIRE_THROWS_AS( tx.sendTo( "x" ), SystemSocket::Error );
         char out[2] = "x";
         int  n = 1;
         REQUIRE_THROWS_AS( tx.sendChunkTo( out, n ), SystemSocket::Error );
         tx.close();
      }

      WHEN( "createSockAddr rejects a bad host" )
      {
         REQUIRE_THROWS_AS( SystemSocket::createSockAddr( 1234, "not-an-ip" ),
                            SystemSocket::Error );
      }
   }

   GIVEN( "TCP error paths on a real connection" )
   {
      SystemSocket server( SystemSocket::Stream, 52058, "127.0.0.1" );
      server.listen();

      SystemSocket client( SystemSocket::Stream, 52058, "127.0.0.1" );
      client.setConnectTimeout( 2000 );
      client.connect();

      SystemSocket accepted;
      server.accept( accepted );

      WHEN( "recv-side timeouts and shutdowns throw" )
      {
         client.setRecvTimeout( 50 );
         char b[8];
         int  n = 8;
         REQUIRE_THROWS_AS( client.recvChunk( b, n ), SystemSocket::Error ); // timeout

         accepted.close();
         REQUIRE_THROWS_AS( client.recv(), SystemSocket::Error ); // peer shutdown
      }

      WHEN( "send to a closed peer throws with SIGPIPE ignored" )
      {
         accepted.close();
         void ( *prev )( int ) = ::signal( SIGPIPE, SIG_IGN );
         bool threw = false;
         try
         {
            for( int i = 0; i < 100 && !threw; ++i ) // Keep sending until the RST arrives.
            {
               client.send( "0123456789abcdef" );
            }
         }
         catch( const SystemSocket::Error & )
         {
            threw = true;
         }
         ::signal( SIGPIPE, prev );
         REQUIRE( threw );
      }

      client.close();
      if( accepted.isValid() )
      {
         accepted.close();
      }
      server.close();
   }

   GIVEN( "option handling and multicast joins that fail for real" )
   {
      WHEN( "getOption/setOption with an invalid option name throw" )
      {
         SystemSocket s( SystemSocket::Datagram, 52059, "127.0.0.1" );
         s.create();

         int nVal = 0;
         unsigned int uiLen = sizeof( nVal );
         REQUIRE_THROWS_AS( s.getOption( SOL_SOCKET, -1, &nVal, uiLen ), SystemSocket::Error );
         REQUIRE_THROWS_AS( s.setOption( SOL_SOCKET, -1, &nVal, sizeof( nVal ) ),
                            SystemSocket::Error );
         s.close();
      }

      WHEN( "joining with an empty group address fails in setsockopt" )
      {
         SystemSocket mc( SystemSocket::enumMulticast, 52060, "" );
         REQUIRE_THROWS_AS( mc.join(), SystemSocket::Error ); // The group resolves to INADDR_NONE.
         mc.close();
      }
   }
}

} //namespace SystemSocket_test
