/** \file ioDevice_test.cpp
  * \brief Catch2 tests for the MagAOX::app::dev::ioDevice device mixin.
  *
  */
#include "../../../../tests/catch2/catch.hpp"

#include "../ioDevice.hpp"

using namespace MagAOX::app;

/** \defgroup ioDevice_tests MagAOX::app::dev::ioDevice Unit Tests
  * \ingroup app_dev_unit_tests
  */
namespace ioDevice_tests
{

SCENARIO( "ioDevice default construction", "[ioDevice]" )
{
   GIVEN("a default-constructed ioDevice")
   {
      dev::ioDevice iod;

      WHEN("no configuration has been loaded")
      {
         THEN("the default timeouts are used")
         {
            REQUIRE( iod.m_readTimeout == 1000);
            REQUIRE( iod.m_writeTimeout == 1000);
         }
      }
   }
}

SCENARIO( "ioDevice configuration setup and loading", "[ioDevice]" )
{
   GIVEN("an ioDevice and an application configurator")
   {
      dev::ioDevice iod;
      int rv;

      WHEN("setupConfig is called")
      {
         mx::app::appConfigurator config;
         rv = iod.setupConfig(config);

         THEN("it returns 0")
         {
            REQUIRE( rv == 0);
         }
      }

      WHEN("a config file specifies non-default read and write timeouts")
      {
         mx::app::writeConfigFile( "/tmp/ioDevice_test.conf", {"device",        "device"},
                                                                {"readTimeout",   "writeTimeout"},
                                                                {"2500",          "3700"} );

         mx::app::appConfigurator config;

         rv = iod.setupConfig(config);
         REQUIRE( rv == 0);

         config.readConfig("/tmp/ioDevice_test.conf");

         rv = iod.loadConfig(config);

         THEN("loadConfig returns 0 and updates the timeouts")
         {
            REQUIRE( rv == 0);
            REQUIRE( iod.m_readTimeout == 2500);
            REQUIRE( iod.m_writeTimeout == 3700);
         }
      }

      WHEN("the config file does not specify the timeouts")
      {
         mx::app::writeConfigFile( "/tmp/ioDevice_test_empty.conf", {}, {}, {} );

         mx::app::appConfigurator config;

         rv = iod.setupConfig(config);
         REQUIRE( rv == 0);

         config.readConfig("/tmp/ioDevice_test_empty.conf");

         rv = iod.loadConfig(config);

         THEN("loadConfig returns 0 and the defaults are unchanged")
         {
            REQUIRE( rv == 0);
            REQUIRE( iod.m_readTimeout == 1000);
            REQUIRE( iod.m_writeTimeout == 1000);
         }
      }
   }
}

SCENARIO( "ioDevice application lifecycle functions", "[ioDevice]" )
{
   GIVEN("an ioDevice")
   {
      dev::ioDevice iod;

      WHEN("appStartup is called")
      {
         int rv = iod.appStartup();

         THEN("it returns 0")
         {
            REQUIRE( rv == 0);
         }
      }

      WHEN("appLogic is called")
      {
         int rv = iod.appLogic();

         THEN("it returns 0")
         {
            REQUIRE( rv == 0);
         }
      }
   }
}

} //namespace ioDevice_tests
