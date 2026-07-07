#include "../../../tests/catch2/catch.hpp"

#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <cstring>
#include <thread>
#include <chrono>

#include "../ttyIOUtils.hpp"
#include "../ttyErrors.hpp"

namespace ttyIOUtils_test
{

SCENARIO( "A string needs to be telnet-ified", "[libMagAOX::tty]" )
{
   GIVEN("Strings in non-telnet format with single chars")
   {
      std::string telnetStr, inputStr;
      int rv;

      WHEN("A single \\r char at end")
      {
         inputStr = "test\r";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\n");
      }

      WHEN("A single \\n char at end")
      {
         inputStr = "test\n";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\n");
      }

      WHEN("A single \\r char in the middle")
      {
         inputStr = "test\rtest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\ntest");
      }

      WHEN("A single \\n char in the middle")
      {
         inputStr = "test\ntest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\ntest");
      }

      WHEN("A single \\r char at the beginning")
      {
         inputStr = "\rtest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest");
      }

      WHEN("A single \\n char at the beginning")
      {
         inputStr = "\ntest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest");
      }
   }

   GIVEN("Strings in non-telnet format with two split chars")
   {
      std::string telnetStr, inputStr;
      int rv;

      WHEN("A single \\r char at end, a \\n at beginning")
      {
         inputStr = "\ntest\r";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\n");
      }

      WHEN("A single \\n char at end, a \\r at beginning")
      {
         inputStr = "\rtest\n";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\n");
      }

      WHEN("A single \\r char in the middle, a \n at beginning")
      {
         inputStr = "\ntest\rtset";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\ntset");
      }

      WHEN("A single \\n char in the middle, a \r at beginning")
      {
         inputStr = "\rtest\ntest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\ntest");
      }

      WHEN("A single \\r char at the beginning, a \\r at end")
      {
         inputStr = "\rtest\r";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\n");
      }

      WHEN("A single \\n char at the beginning, a \\n at end")
      {
         inputStr = "\ntest\r\n";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest\r\n");
      }
   }

   GIVEN("Strings already in telnet format")
   {
      std::string telnetStr, inputStr;
      int rv;

      WHEN("A \\r\\n at end")
      {
         inputStr = "test\r\n";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\n");
      }

      WHEN("A \\r\\n char in the middle")
      {
         inputStr = "test\r\ntest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "test\r\ntest");
      }

      WHEN("A \\r\\n char at the beginning")
      {
         inputStr = "\r\ntest";
         rv = MagAOX::tty::telnetCRLF(telnetStr, inputStr );
         REQUIRE(rv == 0);
         REQUIRE(telnetStr == "\r\ntest");
      }

   }
}

TEST_CASE( "isEndOfTrans checks the tail of the read buffer against the eot string", "[libMagAOX::tty::isEndOfTrans]" )
{
   REQUIRE( MagAOX::tty::isEndOfTrans( "hello> ", "> " ) == true );
   REQUIRE( MagAOX::tty::isEndOfTrans( "hello>x", "> " ) == false );
   REQUIRE( MagAOX::tty::isEndOfTrans( "hi", "hello" ) == false ); // eot longer than strRead
   REQUIRE( MagAOX::tty::isEndOfTrans( "", "" ) == true ); // both empty
}

/// Opens a fresh pty pair, closing the slave end (ttyOpenRaw will reopen it by path).
static std::string openPtySlaveName( int & masterFd )
{
   int slaveFd;
   char name[256];

   REQUIRE( ::openpty( &masterFd, &slaveFd, name, nullptr, nullptr ) == 0 );
   ::close(slaveFd);

   return std::string(name);
}

TEST_CASE( "ttyOpenRaw opens a real tty device and configures it", "[libMagAOX::tty::ttyOpenRaw]" )
{
   SECTION("succeeds on a real pty slave device")
   {
      int masterFd;
      std::string devName = openPtySlaveName(masterFd);

      int fd = -1;
      int rv = MagAOX::tty::ttyOpenRaw(fd, devName, B9600);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(fd > 0);

      ::close(fd);
      ::close(masterFd);
   }

   SECTION("fails with TTY_E_TCGETATTR when the path is not a tty")
   {
      std::string devName = "/tmp/ttyIOUtils_test_regular_file.txt";
      int fdmake = ::open(devName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
      REQUIRE(fdmake >= 0);
      ::close(fdmake);

      int fd = -1;
      int rv = MagAOX::tty::ttyOpenRaw(fd, devName, B9600);

      REQUIRE(rv == TTY_E_TCGETATTR);
      REQUIRE(fd == 0);

      ::unlink(devName.c_str());
   }

   SECTION("fails with TTY_E_TCGETATTR when the device does not exist")
   {
      std::string devName = "/dev/xwctest-no-such-tty-device";

      int fd = -1;
      int rv = MagAOX::tty::ttyOpenRaw(fd, devName, B9600);

      REQUIRE(rv == TTY_E_TCGETATTR);
      REQUIRE(fd == 0);
   }

   SECTION("fails with TTY_E_SETISPEED when an invalid speed is given")
   {
      int masterFd;
      std::string devName = openPtySlaveName(masterFd);

      int fd = -1;
      int rv = MagAOX::tty::ttyOpenRaw(fd, devName, (speed_t) 0xDEADBEEF);

      REQUIRE(rv == TTY_E_SETISPEED);
      REQUIRE(fd == 0);

      ::close(masterFd);
   }
}

/// Makes a non-blocking write-end of a socketpair whose kernel send buffer is filled up so
/// subsequent writes/polls-for-POLLOUT will not be ready. Returns the (still-open) fds.
static void fillSendBuffer( int fd )
{
   int flags = fcntl(fd, F_GETFL, 0);
   fcntl(fd, F_SETFL, flags | O_NONBLOCK);

   char junk[65536];
   memset(junk, 'x', sizeof(junk));
   while(true)
   {
      ssize_t rv = ::write(fd, junk, sizeof(junk));
      if(rv < 0) break;
   }

   fcntl(fd, F_SETFL, flags);
}

TEST_CASE( "ttyWrite writes to a file descriptor", "[libMagAOX::tty::ttyWrite]" )
{
   SECTION("succeeds writing to a socket that's ready and being drained")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::string msg = "hello device";
      int rv = MagAOX::tty::ttyWrite(msg, sp[0], 1000);
      REQUIRE(rv == TTY_E_NOERROR);

      char buff[256];
      ssize_t n = ::recv(sp[1], buff, sizeof(buff), 0);
      REQUIRE(n == (ssize_t) msg.size());
      REQUIRE( std::string(buff, n) == msg );

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_ERRORONWRITE when the descriptor is closed")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      int rv = MagAOX::tty::ttyWrite("data", fd, 1000);
      REQUIRE(rv == TTY_E_ERRORONWRITE);
   }

   SECTION("returns TTY_E_TIMEOUTONWRITE when the elapsed time exceeds a zero timeout")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      int rv = MagAOX::tty::ttyWrite("data", sp[0], 0);
      REQUIRE(rv == TTY_E_TIMEOUTONWRITE);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONWRITEPOLL when the send buffer stays full")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      int sndbuf = 1024;
      ::setsockopt(sp[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

      fillSendBuffer(sp[0]);

      int rv = MagAOX::tty::ttyWrite("more data that won't fit", sp[0], 50);
      REQUIRE(rv == TTY_E_TIMEOUTONWRITEPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }
}

TEST_CASE( "ttyReadRaw reads a raw buffer of bytes from a file descriptor", "[libMagAOX::tty::ttyReadRaw]" )
{
   SECTION("succeeds reading available bytes")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::string msg = "raw bytes";
      REQUIRE( ::send(sp[1], msg.c_str(), msg.size(), 0) == (ssize_t) msg.size() );

      std::vector<unsigned char> vecRead(256);
      int readBytes = -1;
      int rv = MagAOX::tty::ttyReadRaw(vecRead, readBytes, sp[0], 1000);

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(readBytes == (int) msg.size());
      REQUIRE( std::string((char*)vecRead.data(), readBytes) == msg );

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when nothing arrives")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::vector<unsigned char> vecRead(256);
      int readBytes = -1;
      int rv = MagAOX::tty::ttyReadRaw(vecRead, readBytes, sp[0], 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_ERRORONREAD when the descriptor is closed")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      std::vector<unsigned char> vecRead(256);
      int readBytes = -1;
      int rv = MagAOX::tty::ttyReadRaw(vecRead, readBytes, fd, 50);

      REQUIRE(rv == TTY_E_ERRORONREAD);
   }
}

TEST_CASE( "ttyRead(bytes) reads until a specific number of bytes have arrived", "[libMagAOX::tty::ttyRead]" )
{
   SECTION("succeeds, accumulating across multiple sends")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::thread sender([sp]()
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(sp[1], "abc", 3, 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(sp[1], "de", 2, 0);
      });

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, 5, sp[0], 2000);

      sender.join();

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(strRead == "abcde");

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when nothing arrives")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, 5, sp[0], 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when the byte count is never reached")
   {
      // Once the initial bytes are consumed, the subsequent poll for more data blocks for
      // the remaining budget and times out -- the overall TTY_E_TIMEOUTONREAD check only
      // fires if the budget is already exhausted at the top of a loop iteration, which
      // can't happen deterministically here since the preceding poll is itself bounded by
      // that same budget.
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      REQUIRE( ::send(sp[1], "ab", 2, 0) == 2 );

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, 5, sp[0], 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_ERRORONREAD when the descriptor is closed")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, 5, fd, 50);

      REQUIRE(rv == TTY_E_ERRORONREAD);
   }
}

TEST_CASE( "ttyRead(eot) reads until an end-of-transmission string is seen", "[libMagAOX::tty::ttyRead]" )
{
   SECTION("succeeds, accumulating across multiple sends until the eot arrives")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::thread sender([sp]()
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(sp[1], "hello ", 6, 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(sp[1], "world> ", 7, 0);
      });

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, std::string("> "), sp[0], 2000);

      sender.join();

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(strRead == "hello world> ");

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when nothing arrives")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, std::string("> "), sp[0], 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_TIMEOUTONREADPOLL when the eot never arrives")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      REQUIRE( ::send(sp[1], "no eot here", 11, 0) == 11 );

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, std::string("> "), sp[0], 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("returns TTY_E_ERRORONREAD when the descriptor is closed")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      std::string strRead;
      int rv = MagAOX::tty::ttyRead(strRead, std::string("> "), fd, 50);

      REQUIRE(rv == TTY_E_ERRORONREAD);
   }
}

TEST_CASE( "ttyWriteRead writes then reads a reply, optionally swallowing the echo", "[libMagAOX::tty::ttyWriteRead]" )
{
   SECTION("succeeds with echo swallowing, as a real echoing console would behave")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::string strWrite = "cmd";

      std::thread device([sp, strWrite]()
      {
         // Read the command the console echoes back to us.
         char buff[256];
         ssize_t n = ::recv(sp[1], buff, sizeof(buff), 0);
         REQUIRE(n == (ssize_t) strWrite.size());

         // Echo it back. The swallow loop reads while totrv <= strWrite.size(), so it
         // always consumes one byte beyond the echo itself and discards whatever chunk
         // that byte arrived in -- send a throwaway filler byte on its own (separated by a
         // sleep so it isn't coalesced with the echo or the real reply into one read()),
         // then send the real reply afterward for the subsequent ttyRead() to pick up.
         ::send(sp[1], strWrite.c_str(), strWrite.size(), 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         ::send(sp[1], "X", 1, 0);
         std::this_thread::sleep_for(std::chrono::milliseconds(20));
         std::string reply = "result> ";
         ::send(sp[1], reply.c_str(), reply.size(), 0);
      });

      std::string strRead;
      int rv = MagAOX::tty::ttyWriteRead(strRead, strWrite, "> ", true, sp[0], 1000, 1000);

      device.join();

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(strRead == "result> ");

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("succeeds without echo swallowing")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      std::thread device([sp]()
      {
         char buff[256];
         ::recv(sp[1], buff, sizeof(buff), 0);

         std::string reply = "result> ";
         ::send(sp[1], reply.c_str(), reply.size(), 0);
      });

      std::string strRead;
      int rv = MagAOX::tty::ttyWriteRead(strRead, "cmd", "> ", false, sp[0], 1000, 1000);

      device.join();

      REQUIRE(rv == TTY_E_NOERROR);
      REQUIRE(strRead == "result> ");

      ::close(sp[0]);
      ::close(sp[1]);
   }

   SECTION("propagates a write error without attempting the read")
   {
      int fd = ::dup(STDIN_FILENO);
      REQUIRE(fd >= 0);
      ::close(fd);

      std::string strRead;
      int rv = MagAOX::tty::ttyWriteRead(strRead, "cmd", "> ", false, fd, 1000, 1000);

      REQUIRE(rv == TTY_E_ERRORONWRITE);
   }

   SECTION("returns a read error when swallowing the echo fails")
   {
      int sp[2];
      REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

      // Nothing is ever sent back, and the read timeout is short.
      std::string strRead;
      int rv = MagAOX::tty::ttyWriteRead(strRead, "cmd", "> ", true, sp[0], 1000, 50);

      REQUIRE(rv == TTY_E_TIMEOUTONREADPOLL);

      ::close(sp[0]);
      ::close(sp[1]);
   }
}

} //namespace ttyIOUtils_test
