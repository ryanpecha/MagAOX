/** \file netSerial_test.cpp
  * \brief Catch2 tests for netSerial
  */
#include "../../../tests/catch2/catch.hpp"

#include <cstring>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// netSerial keeps its socket fd protected; give the tests direct access to it the same way
// this codebase's other socket-based tests reach into private/protected transport state.
#define protected public
#include "../netSerial.hpp"
#undef protected

namespace libXWCTest
{
namespace netSerialTest
{

TEST_CASE( "netSerial starts with no open socket", "[libMagAOX::tty::netSerial]" )
{
    MagAOX::tty::netSerial ns;
    REQUIRE( ns.getSocketFD() == -1 );
}

TEST_CASE( "netSerial::serialInit", "[libMagAOX::tty::netSerial]" )
{
    SECTION( "succeeds against a real listening server" )
    {
        int listenSock = ::socket( AF_INET, SOCK_STREAM, 0 );
        REQUIRE( listenSock >= 0 );

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
        addr.sin_port = 0;

        REQUIRE( ::bind( listenSock, (struct sockaddr *)&addr, sizeof(addr) ) == 0 );
        REQUIRE( ::listen( listenSock, 1 ) == 0 );

        socklen_t len = sizeof(addr);
        REQUIRE( ::getsockname( listenSock, (struct sockaddr *)&addr, &len ) == 0 );
        uint16_t port = ntohs(addr.sin_port);

        std::thread acceptThread([listenSock]()
        {
            int connFd = ::accept(listenSock, nullptr, nullptr);
            if(connFd >= 0) ::close(connFd);
        });

        MagAOX::tty::netSerial ns;
        int rv = ns.serialInit("127.0.0.1", port);

        acceptThread.join();
        ::close(listenSock);

        REQUIRE( rv == NETSERIAL_E_NOERROR );
        REQUIRE( ns.getSocketFD() > 0 );

        ns.serialClose();
    }

    SECTION( "fails with NETSERIAL_E_CONNECT when the connection is refused" )
    {
        // Bind to an ephemeral port then close it immediately -- nothing is listening, so
        // connecting to it on loopback fails fast with ECONNREFUSED rather than hanging.
        int probe = ::socket( AF_INET, SOCK_STREAM, 0 );
        REQUIRE( probe >= 0 );

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
        addr.sin_port = 0;

        REQUIRE( ::bind( probe, (struct sockaddr *)&addr, sizeof(addr) ) == 0 );

        socklen_t len = sizeof(addr);
        REQUIRE( ::getsockname( probe, (struct sockaddr *)&addr, &len ) == 0 );
        uint16_t port = ntohs(addr.sin_port);

        REQUIRE( ::close(probe) == 0 );

        MagAOX::tty::netSerial ns;
        int rv = ns.serialInit("127.0.0.1", port);

        REQUIRE( rv == NETSERIAL_E_CONNECT );
    }

    SECTION( "fails with NETSERIAL_E_NETWORK when socket() itself fails" )
    {
        struct rlimit orig;
        REQUIRE( getrlimit(RLIMIT_NOFILE, &orig) == 0 );

        struct rlimit tiny;
        tiny.rlim_cur = 3; // stdin/stdout/stderr only -- no room for a new socket fd
        tiny.rlim_max = orig.rlim_max;
        REQUIRE( setrlimit(RLIMIT_NOFILE, &tiny) == 0 );

        MagAOX::tty::netSerial ns;
        int rv = ns.serialInit("127.0.0.1", 12345);

        REQUIRE( setrlimit(RLIMIT_NOFILE, &orig) == 0 );

        REQUIRE( rv == NETSERIAL_E_NETWORK );
    }

    SECTION( "closes a previously-open socket before reopening" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        // connect() will fail (nothing listening), but it must run serialClose() on
        // m_sockfd first -- verified by the peer observing a clean disconnect.
        int rv = ns.serialInit("127.0.0.1", 1);

        REQUIRE( rv != NETSERIAL_E_NOERROR );

        char buff[8];
        REQUIRE( ::recv(sp[1], buff, sizeof(buff), 0) == 0 );

        ::close(sp[1]);
    }
}

TEST_CASE( "netSerial::serialClose", "[libMagAOX::tty::netSerial]" )
{
    SECTION( "closes an open socket" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        REQUIRE( ns.serialClose() == NETSERIAL_E_NOERROR );

        char buff[8];
        REQUIRE( ::recv(sp[1], buff, sizeof(buff), 0) == 0 );

        ::close(sp[1]);
    }

    SECTION( "is safe to call with no socket ever opened" )
    {
        MagAOX::tty::netSerial ns;
        REQUIRE( ns.serialClose() == NETSERIAL_E_NOERROR );
    }
}

TEST_CASE( "netSerial::serialOut", "[libMagAOX::tty::netSerial]" )
{
    SECTION( "sends all the requested bytes" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        std::string msg = "hello device";
        int rv = ns.serialOut(msg.c_str(), msg.size());
        REQUIRE( rv == NETSERIAL_E_NOERROR );

        char buff[256];
        ssize_t n = ::recv(sp[1], buff, sizeof(buff), 0);
        REQUIRE( n == (ssize_t) msg.size() );
        REQUIRE( std::string(buff, n) == msg );

        ::close(sp[0]);
        ::close(sp[1]);
    }

    SECTION( "returns NETSERIAL_E_COMM when the socket is closed" )
    {
        int fd = ::dup(STDIN_FILENO);
        REQUIRE( fd >= 0 );
        ::close(fd);

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = fd;

        int rv = ns.serialOut("data", 4);
        REQUIRE( rv == NETSERIAL_E_COMM );
    }
}

TEST_CASE( "netSerial::serialIn", "[libMagAOX::tty::netSerial]" )
{
    SECTION( "reads exactly the requested number of bytes" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        REQUIRE( ::send(sp[1], "abcde", 5, 0) == 5 );

        char buff[16];
        int rv = ns.serialIn(buff, 5, 1000);

        REQUIRE( rv == 5 );
        REQUIRE( std::string(buff, 5) == "abcde" );

        ::close(sp[0]);
        ::close(sp[1]);
    }

    SECTION( "returns a partial read when no more data arrives before the timeout" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        REQUIRE( ::send(sp[1], "abc", 3, 0) == 3 );

        char buff[16]{};
        int rv = ns.serialIn(buff, 5, 50);

        REQUIRE( rv == 3 );
        REQUIRE( std::string(buff, 3) == "abc" );

        ::close(sp[0]);
        ::close(sp[1]);
    }
}

TEST_CASE( "netSerial::serialInString", "[libMagAOX::tty::netSerial]" )
{
    SECTION( "returns as soon as the terminator is seen" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        REQUIRE( ::send(sp[1], "ok\n", 3, 0) == 3 );

        char buff[16]{};
        int rv = ns.serialInString(buff, sizeof(buff), 1000, '\n');

        REQUIRE( rv == 3 );
        REQUIRE( std::string(buff, 3) == "ok\n" );

        ::close(sp[0]);
        ::close(sp[1]);
    }

    SECTION( "returns whatever arrived once the overall timeout elapses without a terminator" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        std::thread sender([sp]()
        {
            ::send(sp[1], "a", 1, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            ::send(sp[1], "b", 1, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            ::send(sp[1], "c", 1, 0);
        });

        char buff[16]{};
        // No '\n' will ever arrive; the overall 50ms budget elapses partway through.
        int rv = ns.serialInString(buff, sizeof(buff), 50, '\n');

        sender.join();

        REQUIRE( rv >= 1 );
        REQUIRE( rv < (int)sizeof(buff) );

        ::close(sp[0]);
        ::close(sp[1]);
    }

    SECTION( "returns immediately when select itself fails" )
    {
        int fd = ::dup(STDIN_FILENO);
        REQUIRE( fd >= 0 );
        ::close(fd);

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = fd;

        char buff[16]{};
        int rv = ns.serialInString(buff, sizeof(buff), 50, '\n');

        REQUIRE( rv == 0 );
    }

    SECTION( "retries the select() call after being interrupted by a signal" )
    {
        int sp[2];
        REQUIRE( ::socketpair(AF_UNIX, SOCK_STREAM, 0, sp) == 0 );

        MagAOX::tty::netSerial ns;
        ns.m_sockfd = sp[0];

        // A signal handler with no SA_RESTART makes the blocking select() below return
        // EINTR partway through its wait, exercising serialInString's signal-safe retry
        // loop. The terminator arrives well after the signal fires, but still inside the
        // overall timeout, so the retry has to succeed for this to pass.
        struct sigaction sa{};
        sa.sa_handler = []( int ){};
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        struct sigaction old{};
        REQUIRE( ::sigaction(SIGALRM, &sa, &old) == 0 );

        struct itimerval it{};
        it.it_value.tv_usec = 20000; // 20ms
        REQUIRE( ::setitimer(ITIMER_REAL, &it, nullptr) == 0 );

        std::thread sender([sp]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            ::send(sp[1], "z\n", 2, 0);
        });

        char buff[16]{};
        int rv = ns.serialInString(buff, sizeof(buff), 500, '\n');

        sender.join();
        ::sigaction(SIGALRM, &old, nullptr);

        REQUIRE( rv == 2 );
        REQUIRE( std::string(buff, 2) == "z\n" );

        ::close(sp[0]);
        ::close(sp[1]);
    }
}

} // namespace netSerialTest
} // namespace libXWCTest
