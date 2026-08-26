/** \file dssShutter_test.cpp
  * \brief Catch2 tests for the dev::dssShutter mixin.
  *
  * \ingroup testing
  */

//#define CATCH_CONFIG_MAIN
#include "../../../../tests/catch2/catch.hpp"

#include <functional>

#include <mx/sys/timeUtils.hpp>

#include "../../MagAOXApp.hpp"
#include "../dssShutter.hpp"

using namespace MagAOX::app;

/** \defgroup dssShutter_tests libXWC::app::dev::dssShutter Unit Tests
 * \ingroup app_dev_unit_tests
 */
namespace dssShutter_tests
{

/// Test harness for dev::dssShutter
/** Provides the members/methods dssShutter requires of its derived class, plus
  * hooks that let the test suite force specific error paths (INDI property
  * registration failures, thread-start failures) and simulate the digital I/O
  * hardware (trigger/sensor state) that a real shutter would provide.
  *
  * \ingroup dssShutter_tests
  */
struct dssShutterTest : public MagAOXApp<false>, public dev::dssShutter<dssShutterTest>
{
   typedef dev::dssShutter<dssShutterTest> dssShutterT;

   // -- members required by dssShutter via derived() --
   std::string m_shutterStatus;
   int m_shutterState {-2};

   // -- log capture (mirrors the MagAOXApp::log signature so it hides it) --
   static std::string s_lastLogMessage;
   static logPrioT s_lastLogLevel;
   static int s_logCount;

   // -- registerIndiPropertySet control --
   bool m_failPowerRegister {false};
   bool m_failSensorRegister {false};
   bool m_failTriggerRegister {false};

   // -- threadStart control --
   bool m_failOpenThreadStart {false};
   bool m_failShutThreadStart {false};

   // -- recordCamera capture --
   int m_recordCameraCalls {0};
   bool m_lastRecordCameraForce {false};

   // -- sendNewProperty simulated hardware --
   int m_sendNewPropertyCalls {0};
   bool m_triggerResponds {true}; ///< If true, the simulated trigger channel follows the commanded value.
   std::function<void(int)> m_sendNewPropertyHook; ///< Called with the 1-based call number after each sendNewProperty.

   dssShutterTest() : MagAOXApp<false>( "", false )
   {
      m_configName = "dssShutterTest";
      resetLogState();
   }

   ~dssShutterTest() noexcept
   {}

   static void resetLogState()
   {
      s_lastLogMessage.clear();
      s_lastLogLevel = logPrio::LOG_DEFAULT;
      s_logCount = 0;
   }

   /// Capture dssShutter log messages instead of sending them to the normal logger.
   template <typename logT, int retval = 0>
   static int log( const typename logT::messageT &msg, logPrioT level = logPrio::LOG_DEFAULT )
   {
      s_lastLogMessage = logT::msgString( const_cast<uint8_t *>( msg.builder.GetBufferPointer() ), msg.builder.GetSize() );
      s_lastLogLevel = level;
      ++s_logCount;
      return retval;
   }

   int setupConfig( mx::app::appConfigurator & config )
   {
      dssShutterT::setupConfig(config);
      return 0;
   }

   int loadConfig( mx::app::appConfigurator & config )
   {
      dssShutterT::loadConfig(config);
      return 0;
   }

   int appStartup()
   {
      return dssShutterT::appStartup();
   }

   int appLogic()
   {
      return dssShutterT::appLogic();
   }

   int appShutdown()
   {
      return dssShutterT::appShutdown();
   }

   int onPowerOff()
   {
      return dssShutterT::onPowerOff();
   }

   int whilePowerOff()
   {
      return dssShutterT::whilePowerOff();
   }

   /// Hides MagAOXApp<false>::registerIndiPropertySet so appStartup's registration calls can be forced to fail.
   int registerIndiPropertySet( pcf::IndiProperty &prop,
                                 const std::string &devName,
                                 const std::string &propName,
                                 int (*callBack)( void *, const pcf::IndiProperty & )
                               )
   {
      static_cast<void>(callBack);

      prop = pcf::IndiProperty();
      prop.setDevice(devName);
      prop.setName(propName);

      if( &prop == &(this->dssShutterT::m_indiP_powerChannel) && m_failPowerRegister ) return -1;
      if( &prop == &m_indiP_sensorChannel && m_failSensorRegister ) return -1;
      if( &prop == &m_indiP_triggerChannel && m_failTriggerRegister ) return -1;

      return 0;
   }

   /// Hides MagAOXApp<false>::threadStart so the open/shut thread starts can be forced to fail.
   template<class thisPtr, class Function>
   int threadStart( std::thread & thrd,
                     bool & thrdInit,
                     pid_t & tpid,
                     pcf::IndiProperty & thProp,
                     int thrdPrio,
                     const std::string & cpuset,
                     const std::string & thrdName,
                     thisPtr * thrdThis,
                     Function && thrdStart
                   )
   {
      if( thrdName == "open" && m_failOpenThreadStart ) return -1;
      if( thrdName == "shut" && m_failShutThreadStart ) return -1;

      return MagAOXApp<false>::threadStart( thrd, thrdInit, tpid, thProp, thrdPrio, cpuset, thrdName, thrdThis, std::forward<Function>(thrdStart) );
   }

   /// Required by dssShutter via derived(). Simulates the camera-recording side effect.
   int recordCamera( bool force = false )
   {
      ++m_recordCameraCalls;
      m_lastRecordCameraForce = force;
      return 0;
   }

   /// Required by dssShutter via derived(). Simulates the digital I/O hardware.
   template<typename T>
   int sendNewProperty( const pcf::IndiProperty & ipSend, const std::string & el, const T & newVal )
   {
      static_cast<void>(ipSend);
      static_cast<void>(el);

      ++m_sendNewPropertyCalls;

      if( m_triggerResponds )
      {
         m_triggerState = static_cast<int>(newVal);
      }

      if( m_sendNewPropertyHook )
      {
         m_sendNewPropertyHook( m_sendNewPropertyCalls );
      }

      return 0;
   }

   // -- public wrappers around protected dssShutter/MagAOXApp state, for test control --

   void forceShutdown( int v )
   {
      m_shutdown = v;
   }

   void setPowerState( int v ) { dssShutterT::m_powerState = v; }
   int powerState() const { return dssShutterT::m_powerState; }

   void setSensorState( int v ) { m_sensorState = v; }
   int sensorState() const { return m_sensorState; }

   void setTriggerState( int v ) { m_triggerState = v; }
   int triggerState() const { return m_triggerState; }

   void setTriggerResponds( bool v ) { m_triggerResponds = v; }

   void setShutterWait( unsigned v ) { m_shutterWait = v; }
   void setShutterTimeout( unsigned v ) { m_shutterTimeout = v; }

   bool doOpen() const { return m_doOpen; }
   bool doShut() const { return m_doShut; }

   std::string powerDeviceVal() const { return dssShutterT::m_powerDevice; }
   std::string powerChannelVal() const { return dssShutterT::m_powerChannel; }
   std::string dioDeviceVal() const { return m_dioDevice; }
   std::string sensorChannelVal() const { return m_sensorChannel; }
   std::string triggerChannelVal() const { return m_triggerChannel; }
   unsigned shutterWaitVal() const { return m_shutterWait; }
   unsigned shutterTimeoutVal() const { return m_shutterTimeout; }

   pcf::IndiProperty & indiPowerChannel() { return dssShutterT::m_indiP_powerChannel; }
   pcf::IndiProperty & indiSensorChannel() { return m_indiP_sensorChannel; }
   pcf::IndiProperty & indiTriggerChannel() { return m_indiP_triggerChannel; }

   bool openThreadJoinable() { return m_openThread.joinable(); }
   bool shutThreadJoinable() { return m_shutThread.joinable(); }
};

std::string dssShutterTest::s_lastLogMessage;
logPrioT dssShutterTest::s_lastLogLevel = logPrio::LOG_DEFAULT;
int dssShutterTest::s_logCount = 0;

/// Write a complete dssShutter config file with the given wait/timeout values.
void writeShutterConfig( const std::string &wait = "100", const std::string &timeout = "2000" )
{
   mx::app::writeConfigFile( "/tmp/dssShutter_test.conf",
      { "shutter",     "shutter",      "shutter",   "shutter",       "shutter",         "shutter", "shutter" },
      { "powerDevice", "powerChannel", "dioDevice", "sensorChannel", "triggerChannel",  "wait",    "timeout" },
      { "pdu",         "shutterPower", "dio",       "shutterSensor", "shutterTrigger",   wait,      timeout } );
}

/// dssShutter setupConfig/loadConfig
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter configuration", "[dev::dssShutter]" )
{
   GIVEN("a complete shutter config section")
   {
      writeShutterConfig("150", "3000");

      mx::app::appConfigurator config;
      dssShutterTest sh;

      int rv = sh.setupConfig(config);
      REQUIRE( rv == 0 );

      config.readConfig("/tmp/dssShutter_test.conf");

      rv = sh.loadConfig(config);
      REQUIRE( rv == 0 );

      REQUIRE( sh.powerDeviceVal() == "pdu" );
      REQUIRE( sh.powerChannelVal() == "shutterPower" );
      REQUIRE( sh.dioDeviceVal() == "dio" );
      REQUIRE( sh.sensorChannelVal() == "shutterSensor" );
      REQUIRE( sh.triggerChannelVal() == "shutterTrigger" );
      REQUIRE( sh.shutterWaitVal() == 150 );
      REQUIRE( sh.shutterTimeoutVal() == 3000 );
   }
}

/// dssShutter appStartup, normal operation and INDI-registration failures
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter appStartup", "[dev::dssShutter]" )
{
   GIVEN("a fully configured shutter")
   {
      writeShutterConfig("1", "5");

      mx::app::appConfigurator config;

      WHEN("startup succeeds")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);

         int rv = sh.appStartup();
         REQUIRE( rv == 0 );

         REQUIRE( sh.indiPowerChannel().getName() == "shutterPower" );
         REQUIRE( sh.indiSensorChannel().getName() == "shutterSensor" );
         REQUIRE( sh.indiTriggerChannel().getName() == "shutterTrigger" );

         REQUIRE( sh.openThreadJoinable() == true );
         REQUIRE( sh.shutThreadJoinable() == true );

         sh.forceShutdown(1);
         rv = sh.appShutdown();
         REQUIRE( rv == 0 );

         REQUIRE( sh.openThreadJoinable() == false );
         REQUIRE( sh.shutThreadJoinable() == false );
      }

      WHEN("powerChannel registration fails")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);
         sh.m_failPowerRegister = true;

         dssShutterTest::resetLogState();
         int rv = sh.appStartup();
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );

         REQUIRE( sh.openThreadJoinable() == false );
         REQUIRE( sh.shutThreadJoinable() == false );
      }

      WHEN("sensorChannel registration fails")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);
         sh.m_failSensorRegister = true;

         dssShutterTest::resetLogState();
         int rv = sh.appStartup();
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
      }

      WHEN("triggerChannel registration fails")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);
         sh.m_failTriggerRegister = true;

         dssShutterTest::resetLogState();
         int rv = sh.appStartup();
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
      }

      WHEN("open thread fails to start")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);
         sh.m_failOpenThreadStart = true;

         dssShutterTest::resetLogState();
         int rv = sh.appStartup();
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( sh.openThreadJoinable() == false );

         sh.forceShutdown(1);
         sh.appShutdown();
      }

      WHEN("shut thread fails to start")
      {
         dssShutterTest sh;
         sh.setupConfig(config);
         config.readConfig("/tmp/dssShutter_test.conf");
         sh.loadConfig(config);
         sh.m_failShutThreadStart = true;

         dssShutterTest::resetLogState();
         int rv = sh.appStartup();
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );

         // The open thread did start for real in this scenario, clean it up.
         REQUIRE( sh.openThreadJoinable() == true );
         sh.forceShutdown(1);
         sh.appShutdown();
         REQUIRE( sh.openThreadJoinable() == false );
      }
   }
}

/// dssShutter appLogic, onPowerOff, whilePowerOff
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter appLogic", "[dev::dssShutter]" )
{
   GIVEN("a shutter instance")
   {
      dssShutterTest sh;

      WHEN("power state is unknown")
      {
         sh.setPowerState(-1);
         sh.setSensorState(-1);
         int rv = sh.appLogic();
         REQUIRE( rv == 0 );
         REQUIRE( sh.m_shutterStatus == "UNKNOWN" );
         REQUIRE( sh.m_shutterState == -1 );
      }

      WHEN("power is off")
      {
         sh.setPowerState(0);
         int rv = sh.appLogic();
         REQUIRE( rv == 0 );
         REQUIRE( sh.m_shutterStatus == "POWEROFF" );
         REQUIRE( sh.m_shutterState == -1 );
      }

      WHEN("power is on and sensor reads shut")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         int rv = sh.appLogic();
         REQUIRE( rv == 0 );
         REQUIRE( sh.m_shutterStatus == "READY" );
         REQUIRE( sh.m_shutterState == 0 );
      }

      WHEN("power is on and sensor reads open")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         int rv = sh.appLogic();
         REQUIRE( rv == 0 );
         REQUIRE( sh.m_shutterStatus == "READY" );
         REQUIRE( sh.m_shutterState == 1 );
      }

      WHEN("power is on and sensor state is unknown")
      {
         sh.setPowerState(1);
         sh.setSensorState(-1);
         sh.m_shutterState = 42; //sentinel, should not be touched
         int rv = sh.appLogic();
         REQUIRE( rv == 0 );
         REQUIRE( sh.m_shutterStatus == "READY" );
         REQUIRE( sh.m_shutterState == 42 );
      }

      WHEN("onPowerOff and whilePowerOff delegate to appLogic")
      {
         sh.setPowerState(0);
         REQUIRE( sh.onPowerOff() == 0 );
         REQUIRE( sh.m_shutterStatus == "POWEROFF" );

         sh.setPowerState(1);
         sh.setSensorState(1);
         REQUIRE( sh.whilePowerOff() == 0 );
         REQUIRE( sh.m_shutterStatus == "READY" );
         REQUIRE( sh.m_shutterState == 1 );
      }
   }
}

/// dssShutter appShutdown without a prior appStartup
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter appShutdown with no threads started", "[dev::dssShutter]" )
{
   GIVEN("a shutter instance that never called appStartup")
   {
      dssShutterTest sh;

      REQUIRE( sh.openThreadJoinable() == false );
      REQUIRE( sh.shutThreadJoinable() == false );

      int rv = sh.appShutdown();
      REQUIRE( rv == 0 );
   }
}

/// dssShutter setShutterState and the open/shut background threads
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter setShutterState and background threads", "[dev::dssShutter]" )
{
   GIVEN("a running shutter with a stuck trigger (so open/shut always fail)")
   {
      writeShutterConfig("1", "5");

      mx::app::appConfigurator config;

      dssShutterTest sh;
      sh.setupConfig(config);
      config.readConfig("/tmp/dssShutter_test.conf");
      sh.loadConfig(config);

      sh.setPowerState(1);
      sh.setSensorState(0);
      sh.setTriggerState(0);
      sh.setTriggerResponds(false); //hardware never responds -> open()/shut() always fail

      REQUIRE( sh.appStartup() == 0 );

      WHEN("an invalid shutter state is requested")
      {
         dssShutterTest::resetLogState();
         int rv = sh.setShutterState(2);
         REQUIRE( rv == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("invalid shutter request") != std::string::npos );
      }

      WHEN("open is requested and processed by the open thread")
      {
         dssShutterTest::resetLogState();

         int rv = sh.setShutterState(1);
         REQUIRE( rv == 0 );

         bool processed = false;
         for(int i = 0; i < 150 && !processed; ++i)
         {
            mx::sys::milliSleep(20);
            if( !sh.doOpen() ) processed = true;
         }
         REQUIRE( processed == true );

         //open() itself logs a software_error (trigger stuck), and openThreadExec logs
         //another software_error because open() returned < 0.
         REQUIRE( dssShutterTest::s_logCount == 2 );
      }

      WHEN("shut is requested and processed by the shut thread")
      {
         sh.setSensorState(1); //so shut() does not think it is already shut

         dssShutterTest::resetLogState();

         int rv = sh.setShutterState(0);
         REQUIRE( rv == 0 );

         bool processed = false;
         for(int i = 0; i < 150 && !processed; ++i)
         {
            mx::sys::milliSleep(20);
            if( !sh.doShut() ) processed = true;
         }
         REQUIRE( processed == true );

         REQUIRE( dssShutterTest::s_logCount == 2 );
      }

      sh.forceShutdown(1);
      sh.appShutdown();
   }
}

/// dssShutter::open()
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter open", "[dev::dssShutter]" )
{
   GIVEN("a shutter instance")
   {
      dssShutterTest sh;
      sh.setShutterWait(1);
      sh.setShutterTimeout(2);

      WHEN("power is not on")
      {
         sh.setPowerState(0);
         REQUIRE( sh.open() == 0 );
         REQUIRE( sh.m_recordCameraCalls == 0 );
      }

      WHEN("the shutter already reads open")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         REQUIRE( sh.open() == 0 );
         REQUIRE( sh.m_recordCameraCalls == 1 ); //the unconditional recordCamera call
      }

      WHEN("the first attempt succeeds")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         sh.setTriggerState(0);

         dssShutterTest::resetLogState();
         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 1) sh.setSensorState(1);
         };

         REQUIRE( sh.open() == 0 );
         REQUIRE( sh.m_recordCameraCalls == 2 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage == "shutter open" );
      }

      WHEN("the trigger never responds")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         sh.setTriggerState(0);
         sh.setTriggerResponds(false);

         dssShutterTest::resetLogState();

         REQUIRE( sh.open() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter trigger did not change state") != std::string::npos );
      }

      WHEN("the trigger responds but the sensor never confirms, failing entirely")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         dssShutterTest::resetLogState();

         REQUIRE( sh.open() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter failed to open") != std::string::npos );
      }

      WHEN("the trigger sticks after the first attempt, so the second try reports it never changed")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 1) sh.setTriggerResponds(false);
         };

         dssShutterTest::resetLogState();

         REQUIRE( sh.open() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter trigger did not change state") != std::string::npos );
      }

      WHEN("the second attempt succeeds")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 2) sh.setSensorState(1);
         };

         dssShutterTest::resetLogState();

         REQUIRE( sh.open() == 0 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage == "shutter open" );
      }
   }
}

/// dssShutter::shut()
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter shut", "[dev::dssShutter]" )
{
   GIVEN("a shutter instance")
   {
      dssShutterTest sh;
      sh.setShutterWait(1);
      sh.setShutterTimeout(2);

      WHEN("power is not on")
      {
         sh.setPowerState(0);
         REQUIRE( sh.shut() == 0 );
      }

      WHEN("the shutter already reads shut")
      {
         sh.setPowerState(1);
         sh.setSensorState(0);
         REQUIRE( sh.shut() == 0 );
      }

      WHEN("the first attempt succeeds")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         sh.setTriggerState(0);

         dssShutterTest::resetLogState();
         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 1) sh.setSensorState(0);
         };

         REQUIRE( sh.shut() == 0 );
         REQUIRE( sh.m_recordCameraCalls == 1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage == "shutter shut" );
      }

      WHEN("the trigger never responds")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         sh.setTriggerState(0);
         sh.setTriggerResponds(false);

         dssShutterTest::resetLogState();

         REQUIRE( sh.shut() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter trigger did not change state") != std::string::npos );
      }

      WHEN("the trigger responds but the sensor never confirms, failing entirely")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         dssShutterTest::resetLogState();

         REQUIRE( sh.shut() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter failed to shut") != std::string::npos );
      }

      WHEN("the trigger sticks after the first attempt, so the second try reports it never changed")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 1) sh.setTriggerResponds(false);
         };

         dssShutterTest::resetLogState();

         REQUIRE( sh.shut() == -1 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage.find("shutter trigger did not change state") != std::string::npos );
      }

      WHEN("the second attempt succeeds")
      {
         sh.setPowerState(1);
         sh.setSensorState(1);
         sh.setTriggerState(0);
         sh.setTriggerResponds(true);

         sh.m_sendNewPropertyHook = [&sh](int callNum)
         {
            if(callNum == 2) sh.setSensorState(0);
         };

         dssShutterTest::resetLogState();

         REQUIRE( sh.shut() == 0 );
         REQUIRE( dssShutterTest::s_logCount == 1 );
         REQUIRE( dssShutterTest::s_lastLogMessage == "shutter shut" );
      }
   }
}

/// dssShutter INDI callbacks for the power, sensor, and trigger channels
/**
 * \ingroup dssShutter_tests
 */
SCENARIO( "dssShutter INDI callbacks", "[dev::dssShutter]" )
{
   GIVEN("a started shutter with named INDI properties")
   {
      writeShutterConfig("1", "5");

      mx::app::appConfigurator config;

      dssShutterTest sh;
      sh.setupConfig(config);
      config.readConfig("/tmp/dssShutter_test.conf");
      sh.loadConfig(config);
      REQUIRE( sh.appStartup() == 0 );

      WHEN("the power channel callback is exercised")
      {
         pcf::IndiProperty mismatched;
         mismatched.setDevice("other");
         mismatched.setName("other");
         REQUIRE( sh.setCallBack_powerChannel(mismatched) == 0 );
         REQUIRE( sh.powerState() == -1 );

         pcf::IndiProperty ip;
         ip.setDevice("pdu");
         ip.setName("shutterPower");
         REQUIRE( sh.setCallBack_powerChannel(ip) == 0 ); //no "state" element yet
         REQUIRE( sh.powerState() == -1 );

         ip.add( pcf::IndiElement("state", std::string("On")) );
         REQUIRE( sh.setCallBack_powerChannel(ip) == 0 );
         REQUIRE( sh.powerState() == 1 );

         ip["state"].setValue( std::string("Off") );
         REQUIRE( sh.setCallBack_powerChannel(ip) == 0 );
         REQUIRE( sh.powerState() == 0 );

         ip["state"].setValue( std::string("Weird") );
         REQUIRE( sh.setCallBack_powerChannel(ip) == 0 );
         REQUIRE( sh.powerState() == -1 );

         ip["state"].setValue( std::string("On") );
         REQUIRE( dev::dssShutter<dssShutterTest>::st_setCallBack_powerChannel(&sh, ip) == 0 );
         REQUIRE( sh.powerState() == 1 );
      }

      WHEN("the sensor channel callback is exercised")
      {
         pcf::IndiProperty mismatched;
         mismatched.setDevice("dio");
         mismatched.setName("other");
         REQUIRE( sh.setCallBack_sensorChannel(mismatched) == 0 );
         REQUIRE( sh.sensorState() == -1 );

         pcf::IndiProperty ip;
         ip.setDevice("dio");
         ip.setName("shutterSensor");
         REQUIRE( sh.setCallBack_sensorChannel(ip) == 0 ); //no "current" element yet
         REQUIRE( sh.sensorState() == -1 );

         ip.add( pcf::IndiElement("current", (int) 1) );
         REQUIRE( sh.setCallBack_sensorChannel(ip) == 0 );
         REQUIRE( sh.sensorState() == 1 );

         ip["current"].setValue( (int) 0 );
         REQUIRE( sh.setCallBack_sensorChannel(ip) == 0 );
         REQUIRE( sh.sensorState() == 0 );

         ip["current"].setValue( (int) 7 );
         REQUIRE( sh.setCallBack_sensorChannel(ip) == 0 );
         REQUIRE( sh.sensorState() == -1 );

         ip["current"].setValue( (int) 1 );
         REQUIRE( dev::dssShutter<dssShutterTest>::st_setCallBack_sensorChannel(&sh, ip) == 0 );
         REQUIRE( sh.sensorState() == 1 );
      }

      WHEN("the trigger channel callback is exercised")
      {
         pcf::IndiProperty mismatched;
         mismatched.setDevice("dio");
         mismatched.setName("other");
         REQUIRE( sh.setCallBack_triggerChannel(mismatched) == 0 );
         REQUIRE( sh.triggerState() == -1 );

         pcf::IndiProperty ip;
         ip.setDevice("dio");
         ip.setName("shutterTrigger");
         REQUIRE( sh.setCallBack_triggerChannel(ip) == 0 ); //no "current" element yet
         REQUIRE( sh.triggerState() == -1 );

         ip.add( pcf::IndiElement("current", (int) 1) );
         REQUIRE( sh.setCallBack_triggerChannel(ip) == 0 );
         REQUIRE( sh.triggerState() == 1 );

         ip["current"].setValue( (int) 0 );
         REQUIRE( sh.setCallBack_triggerChannel(ip) == 0 );
         REQUIRE( sh.triggerState() == 0 );

         ip["current"].setValue( (int) 7 );
         REQUIRE( sh.setCallBack_triggerChannel(ip) == 0 );
         REQUIRE( sh.triggerState() == -1 );

         ip["current"].setValue( (int) 1 );
         REQUIRE( dev::dssShutter<dssShutterTest>::st_setCallBack_triggerChannel(&sh, ip) == 0 );
         REQUIRE( sh.triggerState() == 1 );
      }

      sh.forceShutdown(1);
      sh.appShutdown();
   }
}

#ifdef DSSSHUTTER_TEST_DOXYGEN_REF
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::setupConfig(*(mx::app::appConfigurator*)0);
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::loadConfig(*(mx::app::appConfigurator*)0);
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::appStartup();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::appLogic();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::appShutdown();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::onPowerOff();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::whilePowerOff();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::setShutterState(0);
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::open();
MagAOX::app::dev::dssShutter<dssShutter_tests::dssShutterTest>::shut();
#endif

} // namespace dssShutter_tests
