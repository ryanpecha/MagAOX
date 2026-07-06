/** \file modbus_exception_test.cpp
 * \brief Catch2 tests for the MagAO-X Modbus exception hierarchy.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#include "../../../tests/catch2/catch.hpp"

#include "../modbus_exception.hpp"

namespace libXWCTest
{
namespace modbusExceptionTest
{

TEST_CASE( "modbus_exception is the default base message and is catchable as std::exception",
           "[modbus_exception]" )
{
    modbus_exception e;

    REQUIRE( std::string( e.what() ) == "A Error In Modbus Happened!" );
    REQUIRE( e.msg == "" );

    bool caught = false;
    try
    {
        throw modbus_exception();
    }
    catch( const std::exception &se )
    {
        caught = true;
        REQUIRE( std::string( se.what() ) == "A Error In Modbus Happened!" );
    }
    REQUIRE( caught == true );
}

TEST_CASE( "modbus_connect_exception uses msg when set, and a default message otherwise",
           "[modbus_exception]" )
{
    SECTION( "default construction, no msg set" )
    {
        modbus_connect_exception e;
        REQUIRE( std::string( e.what() ) == "Having Modbus Connection Problem" );
    }

    SECTION( "msg set explicitly" )
    {
        modbus_connect_exception e;
        e.msg = "recv failed: Connection reset by peer";

        REQUIRE( std::string( e.what() ) == "recv failed: Connection reset by peer" );
    }

    SECTION( "catchable as modbus_exception and std::exception" )
    {
        bool caughtAsBase  = false;
        bool caughtAsStd   = false;

        try
        {
            throw modbus_connect_exception();
        }
        catch( const modbus_exception &me )
        {
            caughtAsBase = true;
            REQUIRE( std::string( me.what() ) == "Having Modbus Connection Problem" );
        }

        try
        {
            throw modbus_connect_exception();
        }
        catch( const std::exception &se )
        {
            caughtAsStd = true;
            static_cast<void>( se );
        }

        REQUIRE( caughtAsBase == true );
        REQUIRE( caughtAsStd == true );
    }
}

TEST_CASE( "modbus_illegal_function_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_illegal_function_exception e;
    REQUIRE( std::string( e.what() ) == "Illegal Function" );
}

TEST_CASE( "modbus_illegal_address_exception sets msg in its constructor but what() ignores it",
           "[modbus_exception]" )
{
    modbus_illegal_address_exception e;

    // The constructor sets msg, but unlike modbus_connect_exception, what() does not consult
    // it -- it always returns the fixed string. Documenting this (mildly surprising) behavior.
    REQUIRE( e.msg == "test" );
    REQUIRE( std::string( e.what() ) == "Illegal Address" );
}

TEST_CASE( "modbus_illegal_data_value_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_illegal_data_value_exception e;
    REQUIRE( std::string( e.what() ) == "Illegal Data Value" );
}

TEST_CASE( "modbus_server_failure_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_server_failure_exception e;
    REQUIRE( std::string( e.what() ) == "Server Failure" );
}

TEST_CASE( "modbus_acknowledge_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_acknowledge_exception e;
    REQUIRE( std::string( e.what() ) == "Acknowledge" );
}

TEST_CASE( "modbus_server_busy_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_server_busy_exception e;
    REQUIRE( std::string( e.what() ) == "Server Busy" );
}

TEST_CASE( "modbus_gateway_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_gateway_exception e;
    REQUIRE( std::string( e.what() ) == "Gateway Problem" );
}

TEST_CASE( "modbus_buffer_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_buffer_exception e;
    REQUIRE( std::string( e.what() ) == "Size of Buffer Is too Small!" );
}

TEST_CASE( "modbus_amount_exception reports a fixed message", "[modbus_exception]" )
{
    modbus_amount_exception e;
    REQUIRE( std::string( e.what() ) == "Too many Data!" );
}

} // namespace modbusExceptionTest
} // namespace libXWCTest
