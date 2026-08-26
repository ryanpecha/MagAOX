/** \file runCommand_test.cpp
  * \brief Catch2 tests for runCommand
  */
#include "../../../tests/catch2/catch.hpp"

#include <sys/resource.h>

#include "../runCommand.hpp"

namespace libXWCTest
{
namespace sysTest
{

TEST_CASE( "runCommand runs a command and captures its output", "[libMagAOX::sys::runCommand]" )
{
    SECTION( "captures stdout" )
    {
        std::vector<std::string> out, err;
        std::vector<std::string> cmd{ "/bin/echo", "hello", "world" };

        int rv = MagAOX::sys::runCommand( out, err, cmd );

        REQUIRE( rv == 0 );
        REQUIRE( out.size() == 1 );
        REQUIRE( out[0] == "hello world" );
        REQUIRE( err.size() == 0 );
    }

    SECTION( "captures stderr" )
    {
        std::vector<std::string> out, err;
        // ls on a nonexistent path execs fine but writes its complaint to stderr.
        std::vector<std::string> cmd{ "/bin/ls", "/no/such/path/xwctest" };

        int rv = MagAOX::sys::runCommand( out, err, cmd );

        REQUIRE( rv == 0 );
        REQUIRE( out.size() == 0 );
        REQUIRE( err.size() == 1 );
    }

    SECTION( "captures multiple lines of stdout" )
    {
        std::vector<std::string> out, err;
        std::vector<std::string> cmd{ "/bin/printf", "one\ntwo\nthree\n" };

        int rv = MagAOX::sys::runCommand( out, err, cmd );

        REQUIRE( rv == 0 );
        REQUIRE( out.size() == 3 );
        REQUIRE( out[0] == "one" );
        REQUIRE( out[1] == "two" );
        REQUIRE( out[2] == "three" );
    }
}

TEST_CASE( "runCommand reports an error when it can't create its pipes", "[libMagAOX::sys::runCommand]" )
{
    struct rlimit orig;
    REQUIRE( getrlimit( RLIMIT_NOFILE, &orig ) == 0 );

    SECTION( "the stdout pipe fails" )
    {
        struct rlimit tiny;
        tiny.rlim_cur = 3; // stdin/stdout/stderr only -- no room for the first pipe's fds
        tiny.rlim_max = orig.rlim_max;
        REQUIRE( setrlimit( RLIMIT_NOFILE, &tiny ) == 0 );

        std::vector<std::string> out, err;
        std::vector<std::string> cmd{ "/bin/echo", "unreachable" };
        int                      rv = MagAOX::sys::runCommand( out, err, cmd );

        REQUIRE( setrlimit( RLIMIT_NOFILE, &orig ) == 0 );

        REQUIRE( rv == -1 );
        REQUIRE( out.size() == 1 );
    }

    SECTION( "the stderr pipe fails" )
    {
        struct rlimit tiny;
        tiny.rlim_cur = 5; // room for exactly the first pipe's two fds, not the second's
        tiny.rlim_max = orig.rlim_max;
        REQUIRE( setrlimit( RLIMIT_NOFILE, &tiny ) == 0 );

        std::vector<std::string> out, err;
        std::vector<std::string> cmd{ "/bin/echo", "unreachable" };
        int                      rv = MagAOX::sys::runCommand( out, err, cmd );

        REQUIRE( setrlimit( RLIMIT_NOFILE, &orig ) == 0 );

        REQUIRE( rv == -1 );
        REQUIRE( out.size() == 1 );
    }
}

TEST_CASE( "runCommand reports an error when fork() fails", "[libMagAOX::sys::runCommand]" )
{
    struct rlimit orig;
    REQUIRE( getrlimit( RLIMIT_NPROC, &orig ) == 0 );

    struct rlimit tiny;
    tiny.rlim_cur = 1; // this process alone already exceeds this, so the next fork() fails
    tiny.rlim_max = orig.rlim_max;
    REQUIRE( setrlimit( RLIMIT_NPROC, &tiny ) == 0 );

    std::vector<std::string> out, err;
    std::vector<std::string> cmd{ "/bin/echo", "unreachable" };
    int                      rv = MagAOX::sys::runCommand( out, err, cmd );

    REQUIRE( setrlimit( RLIMIT_NPROC, &orig ) == 0 );

    REQUIRE( rv == -1 );
    REQUIRE( out.size() == 1 );
}

} // namespace sysTest
} // namespace libXWCTest
