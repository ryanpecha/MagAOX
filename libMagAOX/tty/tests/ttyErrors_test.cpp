/** \file ttyErrors_test.cpp
  * \brief Catch2 tests for ttyErrorString
  */
#include "../../../tests/catch2/catch.hpp"

#include "../ttyErrors.hpp"

namespace libXWCTest
{
namespace ttyTest
{

TEST_CASE( "ttyErrorString maps every known error code to a non-empty message", "[libMagAOX::tty::ttyErrorString]" )
{
    int codes[] = { TTY_E_NOERROR,
                    TTY_E_TCGETATTR,
                    TTY_E_SETISPEED,
                    TTY_E_SETOSPEED,
                    TTY_E_TCSETATTR,
                    TTY_E_TIMEOUTONWRITEPOLL,
                    TTY_E_ERRORONWRITEPOLL,
                    TTY_E_ERRORONWRITE,
                    TTY_E_TIMEOUTONWRITE,
                    TTY_E_TIMEOUTONREADPOLL,
                    TTY_E_ERRORONREADPOLL,
                    TTY_E_ERRORONREAD,
                    TTY_E_TIMEOUTONREAD,
                    TTY_E_NODEVNAMES,
                    TTY_E_UDEVNEWFAILED,
                    TTY_E_DEVNOTFOUND,
                    TTY_E_BADBAUDRATE,
                    TELNET_E_GETADDR,
                    TELNET_E_SOCKET,
                    TELNET_E_BIND,
                    TELNET_E_CONNECT,
                    TELNET_E_TELNETINIT,
                    TELNET_E_EHERROR,
                    TELNET_E_LOGINTIMEOUT };

    for( int ec : codes )
    {
        INFO( "error code " << ec );
        REQUIRE( MagAOX::tty::ttyErrorString( ec ) != "" );
    }
}

TEST_CASE( "ttyErrorString falls back to an unknown-error message for an unrecognized code", "[libMagAOX::tty::ttyErrorString]" )
{
    REQUIRE( MagAOX::tty::ttyErrorString( -1 ) == "TTY: unknown error code" );
}

} // namespace ttyTest
} // namespace libXWCTest
