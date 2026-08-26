/** \file telnetConn_test.cpp
  * \brief Catch2 tests for telnetConn
  */
#include "../../../tests/catch2/catch.hpp"

#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <future>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../telnetConn.hpp"

namespace libXWCTest
{
namespace telnetConnTest
{

/// Binds a listening socket on loopback with an OS-chosen port, and hands back a background
/// thread that will accept exactly one connection and deliver its fd through the future.
struct FakeServer
{
   int listenSock;
   uint16_t port;
   std::future<int> connFd;
   std::thread acceptThread;

   FakeServer()
   {
      listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      addr.sin_port = 0;

      ::bind(listenSock, (struct sockaddr*)&addr, sizeof(addr));
      ::listen(listenSock, 1);

      socklen_t len = sizeof(addr);
      ::getsockname(listenSock, (struct sockaddr*)&addr, &len);
      port = ntohs(addr.sin_port);

      std::promise<int> prom;
      connFd = prom.get_future();
      acceptThread = std::thread([this, p = std::move(prom)]() mutable
      {
         int fd = ::accept(listenSock, nullptr, nullptr);
         p.set_value(fd);
      });
   }

   ~FakeServer()
   {
      if(acceptThread.joinable()) acceptThread.join();
      ::close(listenSock);
   }
};

/// Fills the kernel send buffer on fd so subsequent POLLOUT waits won't be ready.
static void fillSendBuffer( int fd )
{
   int flags = fcntl(fd, F_GETFL, 0);
   fcntl(fd, F_SETFL, flags | O_NONBLOCK);

   char junk[65536];
   memset(junk, 'x', sizeof(junk));
   while( ::write(fd, junk, sizeof(junk)) > 0 ) {}

   fcntl(fd, F_SETFL, flags);
}

TEST_CASE( "telnetConn::connect", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("succeeds against a real listening server")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      int rv = tc.connect("127.0.0.1", std::to_string(server.port));

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_sock > 0);
      REQUIRE(tc.m_telnet != nullptr);

      ::close(connFd);
   }

   SECTION("fails with TELNET_E_GETADDR for an unresolvable host/port combination")
   {
      MagAOX::tty::telnetConn tc;
      // An empty host with a numeric-looking service and no AI_PASSIVE flag is rejected by
      // getaddrinfo immediately (EAI_NONAME), without any network access being attempted.
      int rv = tc.connect("", "12345");

      REQUIRE(rv == TELNET_E_GETADDR);
   }

   SECTION("fails with TELNET_E_SOCKET when socket() itself fails")
   {
      struct rlimit orig;
      REQUIRE( getrlimit(RLIMIT_NOFILE, &orig) == 0 );

      struct rlimit tiny;
      tiny.rlim_cur = 3;
      tiny.rlim_max = orig.rlim_max;
      REQUIRE( setrlimit(RLIMIT_NOFILE, &tiny) == 0 );

      MagAOX::tty::telnetConn tc;
      int rv = tc.connect("127.0.0.1", "12345");

      REQUIRE( setrlimit(RLIMIT_NOFILE, &orig) == 0 );

      REQUIRE(rv == TELNET_E_SOCKET);
   }

   SECTION("fails with TELNET_E_CONNECT when the connection is refused")
   {
      int probe = ::socket(AF_INET, SOCK_STREAM, 0);
      REQUIRE(probe >= 0);

      struct sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      addr.sin_port = 0;

      REQUIRE( ::bind(probe, (struct sockaddr*)&addr, sizeof(addr)) == 0 );

      socklen_t len = sizeof(addr);
      REQUIRE( ::getsockname(probe, (struct sockaddr*)&addr, &len) == 0 );
      uint16_t port = ntohs(addr.sin_port);

      REQUIRE( ::close(probe) == 0 );

      MagAOX::tty::telnetConn tc;
      int rv = tc.connect("127.0.0.1", std::to_string(port));

      REQUIRE(rv == TELNET_E_CONNECT);
   }
}

TEST_CASE( "telnetConn destructor cleans up an open connection", "[libMagAOX::tty::telnetConn]" )
{
   FakeServer server;
   int connFd;

   {
      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      connFd = server.connFd.get();
   } // tc destructs here: telnet_free() + close(m_sock)

   char buff[8];
   REQUIRE( ::recv(connFd, buff, sizeof(buff), 0) == 0 ); // peer sees clean EOF
   ::close(connFd);
}

TEST_CASE( "telnetConn::noLogin marks the connection as logged in", "[libMagAOX::tty::telnetConn]" )
{
   MagAOX::tty::telnetConn tc;
   REQUIRE( tc.noLogin() == TTY_E_NOERROR );
   REQUIRE( tc.m_loggedin == 5 ); // TELNET_LOGGED_IN
}

TEST_CASE( "telnetConn::login walks the username/password/prompt handshake", "[libMagAOX::tty::telnetConn]" )
{
   FakeServer server;

   MagAOX::tty::telnetConn tc;
   REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

   int connFd = server.connFd.get();
   REQUIRE(connFd >= 0);

   std::thread device([connFd]()
   {
      char buff[256];

      // Prompt for username, then wait for the client to send it.
      std::string prompt1 = "Username:";
      ::send(connFd, prompt1.c_str(), prompt1.size(), 0);
      REQUIRE( ::recv(connFd, buff, sizeof(buff), 0) > 0 );

      // Prompt for password, then wait for the client to send it.
      std::string prompt2 = "Password:";
      ::send(connFd, prompt2.c_str(), prompt2.size(), 0);
      REQUIRE( ::recv(connFd, buff, sizeof(buff), 0) > 0 );

      // Finally send the shell prompt to complete the login.
      std::string prompt3 = "$> ";
      ::send(connFd, prompt3.c_str(), prompt3.size(), 0);
   });

   int rv = tc.login("theuser", "thepass");

   device.join();
   ::close(connFd);

   REQUIRE(rv == TTY_E_NOERROR);
   REQUIRE(tc.m_loggedin == 5); // TELNET_LOGGED_IN
}

TEST_CASE( "telnetConn::login handles a peer that disconnects", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("a clean close (recv returns 0) ends the loop and reports success")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);
      ::close(connFd); // closes without ever sending the username prompt

      // Per the current implementation, an EOF here just breaks out of the poll loop and
      // falls through to a success return -- it does not itself confirm a completed login
      // (m_loggedin never advances past TELNET_WAITING_USER).
      int rv = tc.login("theuser", "thepass");

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_loggedin == 0); // still TELNET_WAITING_USER
   }

   SECTION("a reset connection (recv error) is reported as TTY_E_ERRORONREAD")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      // SO_LINGER{on,0} makes close() emit an RST instead of a graceful FIN, so the
      // client's subsequent recv() fails with ECONNRESET instead of seeing a clean EOF.
      struct linger lo{1, 0};
      ::setsockopt(connFd, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));
      ::close(connFd);

      int rv = tc.login("theuser", "thepass");

      REQUIRE(rv == TTY_E_ERRORONREAD);
   }
}

TEST_CASE( "telnetConn::write", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("sends the (telnet-CRLF-ified) buffer to the peer")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      int rv = tc.write("hello", 1000);
      REQUIRE(rv == TTY_E_NOERROR);

      char buff[256];
      ssize_t n = ::recv(connFd, buff, sizeof(buff), 0);
      REQUIRE(n == 5);
      REQUIRE( std::string(buff, n) == "hello" );

      ::close(connFd);
   }

   SECTION("returns TTY_E_TIMEOUTONWRITE when the elapsed time exceeds a zero timeout")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      int rv = tc.write("data", 0);
      REQUIRE(rv == TTY_E_TIMEOUTONWRITE);

      ::close(connFd);
   }

   SECTION("returns TTY_E_TIMEOUTONWRITEPOLL when the send buffer stays full")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      // Unlike a unix-domain socketpair, a TCP loopback connection's kernel will keep
      // draining the sender's buffer into the peer's receive buffer on its own (regardless
      // of whether anyone calls recv()) -- so the peer's receive buffer needs shrinking too,
      // or the "full" send buffer re-drains before write() gets a chance to poll it.
      int rcvbuf = 1024;
      ::setsockopt(connFd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

      int sndbuf = 1024;
      ::setsockopt(tc.m_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
      fillSendBuffer(tc.m_sock);

      int rv = tc.write("more data that will not fit", 50);
      REQUIRE(rv == TTY_E_TIMEOUTONWRITEPOLL);

      ::close(connFd);
   }
}

TEST_CASE( "telnetConn::read(eot)", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("accumulates m_strRead until the eot is seen, clearing by default")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      // The event handler only accumulates data into m_strRead once logged in -- otherwise
      // it treats every chunk as a candidate username/password/prompt match instead.
      tc.noLogin();
      tc.m_strRead = "stale data that should be cleared";

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      std::thread device([connFd]()
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         std::string msg = "hello world> ";
         ::send(connFd, msg.c_str(), msg.size(), 0);
      });

      int rv = tc.read("> ", 2000);

      device.join();
      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == "hello world> ");
   }

   SECTION("appends instead of clearing when clear=false")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();
      tc.m_strRead = "prefix:";

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      std::thread device([connFd]()
      {
         std::string msg = "suffix> ";
         ::send(connFd, msg.c_str(), msg.size(), 0);
      });

      int rv = tc.read("> ", 2000, false);

      device.join();
      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == "prefix:suffix> ");
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when nothing arrives")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      int rv = tc.read("> ", 50);
      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(connFd);
   }

   SECTION("returns TTY_E_ERRORONREAD when the socket is closed out from under it")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);
      ::close(connFd);

      ::close(tc.m_sock); // leave tc.m_sock as a stale, closed descriptor number
      int rv = tc.read("> ", 50);
      REQUIRE(rv == TTY_E_ERRORONREAD);

      tc.m_sock = 0; // prevent the destructor from closing an unrelated, possibly-reused fd
   }

   SECTION("accumulates across multiple chunks before the eot arrives")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      std::thread device([connFd]()
      {
         ::send(connFd, "hello ", 6, 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(connFd, "world> ", 7, 0);
      });

      int rv = tc.read("> ", 2000);

      device.join();
      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == "hello world> ");
   }

   SECTION("strips leading control bytes and turns embedded NULs into newlines")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      unsigned char raw[] = { 5, 5, 'h', 'e', 'l', 'l', 'o', 0, 'w', 'o', 'r', 'l', 'd', '>', ' ' };
      ::send(connFd, raw, sizeof(raw), 0);

      int rv = tc.read("> ", 2000);

      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == std::string("hello\nworld> "));
   }
}

TEST_CASE( "telnetConn::read(timeout) uses m_prompt as the eot", "[libMagAOX::tty::telnetConn]" )
{
   FakeServer server;

   MagAOX::tty::telnetConn tc;
   REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
   tc.noLogin();
   tc.m_prompt = "custom% ";

   int connFd = server.connFd.get();
   REQUIRE(connFd >= 0);

   std::thread device([connFd]()
   {
      std::string msg = "response custom% ";
      ::send(connFd, msg.c_str(), msg.size(), 0);
   });

   int rv = tc.read(2000);

   device.join();
   ::close(connFd);

   REQUIRE(rv == TTY_E_NOERROR);
   REQUIRE(tc.m_strRead == "response custom% ");
}

TEST_CASE( "telnetConn::writeRead", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("succeeds with echo swallowing")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();
      tc.m_prompt = "> "; // writeRead() always reads for m_prompt, not a passed-in eot

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      std::string strWrite = "cmd";

      std::thread device([connFd, strWrite]()
      {
         char buff[256];
         ssize_t n = ::recv(connFd, buff, sizeof(buff), 0);
         REQUIRE(n == (ssize_t) strWrite.size());

         // Unlike ttyIOUtils::ttyWriteRead, the swallow loop here accumulates every chunk
         // straight into m_strRead (via telnet_recv/event_handler) and only erases the
         // first strWrite.size() characters once it has more than that many -- so sending
         // the echo, then (after a delay, so it lands in a separate read()) the real reply
         // in one piece, leaves exactly the reply behind after the erase.
         ::send(connFd, strWrite.c_str(), strWrite.size(), 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         std::string reply = "result> ";
         ::send(connFd, reply.c_str(), reply.size(), 0);
      });

      int rv = tc.writeRead(strWrite, true, 1000, 1000);

      device.join();
      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == "result> ");
   }

   SECTION("succeeds without echo swallowing")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );
      tc.noLogin();
      tc.m_prompt = "> ";

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      std::thread device([connFd]()
      {
         char buff[256];
         ::recv(connFd, buff, sizeof(buff), 0);

         std::string reply = "result> ";
         ::send(connFd, reply.c_str(), reply.size(), 0);
      });

      int rv = tc.writeRead("cmd", false, 1000, 1000);

      device.join();
      ::close(connFd);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(tc.m_strRead == "result> ");
   }

   SECTION("propagates a write timeout without attempting the read")
   {
      FakeServer server;

      MagAOX::tty::telnetConn tc;
      REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

      int connFd = server.connFd.get();
      REQUIRE(connFd >= 0);

      int rv = tc.writeRead("cmd", false, 0, 1000);
      REQUIRE(rv == TTY_E_TIMEOUTONWRITE);

      ::close(connFd);
   }
}

TEST_CASE( "telnetConn::send (static)", "[libMagAOX::tty::telnetConn]" )
{
   SECTION("sends all the requested bytes")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      int rv = MagAOX::tty::telnetConn::send(sp[0], "abc", 3);
      REQUIRE(rv == TTY_E_NOERROR);

      char buff[8];
      REQUIRE( ::recv(sp[1], buff, sizeof(buff), 0) == 3 );

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_ERRORONWRITE when the descriptor is closed")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      int rv = MagAOX::tty::telnetConn::send(fd, "abc", 3);
      REQUIRE(rv == TTY_E_ERRORONWRITE);
   }
}

TEST_CASE( "telnetConn's libtelnet event handler reacts to raw protocol negotiation", "[libMagAOX::tty::telnetConn]" )
{
   FakeServer server;

   MagAOX::tty::telnetConn tc;
   REQUIRE( tc.connect("127.0.0.1", std::to_string(server.port)) == TTY_E_NOERROR );

   int connFd = server.connFd.get();
   REQUIRE(connFd >= 0);

   // Make sure TTYPE negotiation (which replies with getenv("TERM")) can't dereference a
   // null pointer regardless of the ambient environment this test runs in.
   setenv("TERM", "xterm", 1);

   std::thread device([connFd]()
   {
      // libtelnet only fires a NEGOTIATE_EVENT when the option's rfc1143 state actually
      // changes, so WONT/DONT need a prior WILL/DO on the *same* option to have something
      // to revoke:
      //   WILL COMPRESS2 -> him=YES (fires EV_WILL, per telopts' {WONT,DO} stance)
      //   WONT COMPRESS2 -> him was YES, now revoked (fires EV_WONT)
      //   DO TTYPE       -> us=YES (fires EV_DO, per telopts' {WILL,DONT} stance)
      //   DONT TTYPE     -> us was YES, now revoked (fires EV_DONT)
      unsigned char bytes[] = {
         255, 251, 86,  // IAC WILL COMPRESS2
         255, 252, 86,  // IAC WONT COMPRESS2
         255, 253, 24,  // IAC DO TTYPE
         255, 254, 24,  // IAC DONT TTYPE
         255, 250, 24, 1, 255, 240, // IAC SB TTYPE SEND IAC SE
      };
      ::send(connFd, bytes, sizeof(bytes), 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
   });

   // Just drive telnet_recv via a real read; the assertions of interest are that this
   // doesn't crash and that (per the coverage report) the WILL/WONT/DO/DONT/TTYPE branches
   // in the event handler get exercised.
   int rv = tc.read("this eot will not arrive", 100);
   REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

   device.join();
   ::close(connFd);
}

} // namespace telnetConnTest
} // namespace libXWCTest
