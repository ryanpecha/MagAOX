/** \file usbDevice_test.cpp
  * \brief Catch2 tests for usbDevice
  */
#include "../../../tests/catch2/catch.hpp"

#include "../usbDevice.hpp"
#include "../ttyErrors.hpp"

namespace libXWCTest
{
namespace ttyTest
{

TEST_CASE( "usbDevice::loadConfig maps every recognized baud rate string to its speed_t constant",
           "[libMagAOX::tty::usbDevice]" )
{
    std::pair<std::string, speed_t> rates[] = { { "50", B50 },       { "75", B75 },        { "110", B110 },
                                                { "134.5", B134 },  { "150", B150 },      { "200", B200 },
                                                { "300", B300 },    { "600", B600 },      { "1800", B1800 },
                                                { "2400", B2400 },  { "4800", B4800 },    { "19200", B19200 },
                                                { "57600", B57600 },{ "38400", B38400 } };

    for( auto &r : rates )
    {
        INFO( "baud config value " << r.first );

        mx::app::writeConfigFile( "/tmp/usbDevice_test_baud.conf", { "usb" }, { "baud" }, { r.first } );

        mx::app::appConfigurator config;
        MagAOX::tty::usbDevice   dev;

        REQUIRE( dev.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/usbDevice_test_baud.conf" );

        dev.loadConfig( config );

        REQUIRE( dev.m_baudRate == r.second );
    }
}

TEST_CASE( "usbDevice::connect closes an already-open file descriptor before reopening", "[libMagAOX::tty::usbDevice]" )
{
    MagAOX::tty::usbDevice dev;
    dev.m_deviceName = "/dev/xwctest-no-such-tty";
    dev.m_baudRate   = B9600;

    // Any valid, harmless fd stands in for a previously-opened device -- connect() should
    // close it (rather than leak it) before attempting to reopen m_deviceName.
    dev.m_fileDescrip = ::dup( STDIN_FILENO );
    REQUIRE( dev.m_fileDescrip > 0 );

    REQUIRE( dev.connect() != 0 );
    REQUIRE( dev.m_fileDescrip == 0 );
}

TEST_CASE( "usbDevice::setupConfig registers the usb section options", "[libMagAOX::tty::usbDevice]" )
{
    mx::app::appConfigurator config;
    MagAOX::tty::usbDevice   dev;

    REQUIRE( dev.setupConfig( config ) == 0 );
}

TEST_CASE( "usbDevice::loadConfig reads vendor/product/serial/baud and rejects a bad baud rate",
           "[libMagAOX::tty::usbDevice]" )
{
    SECTION( "a recognized baud rate is accepted and getDeviceName() runs against real udev" )
    {
        mx::app::writeConfigFile( "/tmp/usbDevice_test.conf",
                                  { "usb", "usb", "usb", "usb" },
                                  { "idVendor", "idProduct", "serial", "baud" },
                                  { "ffff", "ffff", "XWCTEST-NO-SUCH-SERIAL", "9600" } );

        mx::app::appConfigurator config;
        MagAOX::tty::usbDevice   dev;

        REQUIRE( dev.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/usbDevice_test.conf" );

        // No real device has this vendor/product/serial, so loadConfig's internal
        // getDeviceName() call legitimately fails to find a match.
        int rv = dev.loadConfig( config );

        REQUIRE( dev.m_idVendor == "ffff" );
        REQUIRE( dev.m_idProduct == "ffff" );
        REQUIRE( dev.m_serial == "XWCTEST-NO-SUCH-SERIAL" );
        REQUIRE( dev.m_baudRate == B9600 );
        REQUIRE( rv != 0 );
    }

    SECTION( "an unrecognized baud rate is rejected before getDeviceName() is even tried" )
    {
        mx::app::writeConfigFile( "/tmp/usbDevice_test.conf", { "usb" }, { "baud" }, { "1234567" } );

        mx::app::appConfigurator config;
        MagAOX::tty::usbDevice   dev;

        REQUIRE( dev.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/usbDevice_test.conf" );

        REQUIRE( dev.loadConfig( config ) == TTY_E_BADBAUDRATE );
    }

    SECTION( "no baud rate configured at all is also rejected" )
    {
        mx::app::appConfigurator config;
        MagAOX::tty::usbDevice   dev;

        REQUIRE( dev.setupConfig( config ) == 0 );

        REQUIRE( dev.loadConfig( config ) == TTY_E_BADBAUDRATE );
    }
}

TEST_CASE( "usbDevice::connect fails cleanly when there is no matching device", "[libMagAOX::tty::usbDevice]" )
{
    MagAOX::tty::usbDevice dev;
    dev.m_deviceName = "/dev/xwctest-no-such-tty";
    dev.m_baudRate   = B9600;

    REQUIRE( dev.connect() != 0 );
    REQUIRE( dev.m_fileDescrip == 0 );
}

} // namespace ttyTest
} // namespace libXWCTest
