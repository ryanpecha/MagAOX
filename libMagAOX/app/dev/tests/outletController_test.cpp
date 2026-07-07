//#define CATCH_CONFIG_MAIN
#include "../../../../tests/catch2/catch.hpp"

#include <mx/sys/timeUtils.hpp>

#define OUTLET_CTRL_TEST_NOINDI
//#define OUTLET_CTRL_TEST_NOLOG
#include "../../MagAOXApp.hpp"
#include "../outletController.hpp"

using namespace MagAOX::app;

/** \defgroup outletController_tests libXWC::app::dev::outletController Unit Tests
 * \ingroup app_dev_unit_tests
*/
namespace outletController_tests
{

struct outletControllerTest : public MagAOXApp<false>, dev::outletController<outletControllerTest>
{
   std::vector<double> m_timestamps;

   /// If set to a valid outlet number, turnOutletOn/turnOutletOff will fail (return -1)
   /// for that outlet number only.  Used to simulate a hardware failure to exercise
   /// the error-return paths in turnChannelOn/turnChannelOff.  -1 (default) means never fail.
   int m_failOutlet {-1};

   outletControllerTest()
        : MagAOX::app::MagAOXApp<false>( "", false )
   {
      setNumberOfOutlets(4);
      m_timestamps.resize(4,0);
      turnOutletOff(0);
      turnOutletOff(1);
      turnOutletOff(2);
      turnOutletOff(3);
   }

   ~outletControllerTest() noexcept
   {}

   int setupConfig( mx::app::appConfigurator & config)
   {
      return dev::outletController<outletControllerTest>::setupConfig(config);
   }

   int loadConfig( mx::app::appConfigurator & config)
   {
      return dev::outletController<outletControllerTest>::loadConfig(config);
   }

   int appStartup()
   {
      return 0;
   }

   int appLogic()
   {
      return 0;
   }

   int appShutdown()
   {
      return 0;
   }

   int updateOutletState( int outletNum )
   {
      return m_outletStates[outletNum];
   }

   int turnOutletOn( int outletNum )
   {
      if(m_failOutlet == outletNum) return -1;

      m_outletStates[outletNum] = 2;
      mx::sys::nanoSleep(1);
      m_timestamps[outletNum] = mx::sys::get_curr_time();

      return 0;
   }

   int turnOutletOff( int outletNum )
   {
      if(m_failOutlet == outletNum) return -1;

      m_outletStates[outletNum] = 0;
      mx::sys::nanoSleep(1);
      m_timestamps[outletNum] = mx::sys::get_curr_time();

      return 0;
   }

};

/// outletController Configuration
/**
 * \ingroup outletController_tests
 */
SCENARIO( "outletController Configuration", "[outletController]" )
{
   GIVEN("a config file with 4 channels for 4 outlets")
   {
      WHEN("using outlet keyword, only outlet specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel2",     "channel3",      "channel4"},
                                                        {"outlet",   "outlet",       "outlet",          "outlet"},
                                                        {"0",         "1",             "2",                "3"} );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 4);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 0 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 1 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel3");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 2 );

         onOrder = pdt.channelOnOrder("channel3");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel3");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel3");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel3");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel4");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 3 );

         onOrder = pdt.channelOnOrder("channel4");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel4");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel4");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel4");
         REQUIRE( offDelays.size() == 0);

      }

      WHEN("using outlet keyword, all specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1", "channel1", "channel1", "channel1",  "channel2",  "channel2", "channel2", "channel2", "channel2",  "channel3", "channel3", "channel3", "channel3", "channel3",   "channel4",  "channel4", "channel4", "channel4", "channel4"  },
                                                        {"outlet",   "onOrder",  "offOrder", "onDelays", "offDelays", "outlet",    "onOrder",  "offOrder", "onDelays", "offDelays", "outlet",   "onOrder",  "offOrder", "onDelays", "offDelays",  "outlet",   "onOrder",  "offOrder", "onDelays", "offDelays"  },
                                                        {"0",        "0",        "0",        "100",      "120",       "1",         "0",        "0",        "105",      "130",       "2",        "0",        "0",        "107",      "132",        "3",      "0",        "0",        "108",      "133"});

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 4);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;

         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 0 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 100);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 120);

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 1 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 105);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 130);

         outlets = pdt.channelOutlets("channel3");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 2 );

         onOrder = pdt.channelOnOrder("channel3");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel3");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel3");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 107);
         offDelays = pdt.channelOffDelays("channel3");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 132);

         outlets = pdt.channelOutlets("channel4");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 3 );

         onOrder = pdt.channelOnOrder("channel4");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel4");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel4");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 108);
         offDelays = pdt.channelOffDelays("channel4");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 133);
      }

      WHEN("using outlets keyword, only outlet specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",     "channel2",     "channel3",      "channel4"},
                                                        {"outlets",       "outlets",       "outlets",          "outlets"},
                                                        {"0",             "1",             "2",                "3"} );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 4);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 0 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 1 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel3");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 2 );

         onOrder = pdt.channelOnOrder("channel3");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel3");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel3");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel3");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel4");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 3 );

         onOrder = pdt.channelOnOrder("channel4");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel4");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel4");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel4");
         REQUIRE( offDelays.size() == 0);
      }

      WHEN("using outlets keyword, all specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1", "channel1", "channel1", "channel1",  "channel2", "channel2", "channel2", "channel2", "channel2",  "channel3", "channel3", "channel3", "channel3", "channel3",   "channel4",  "channel4", "channel4", "channel4", "channel4"  },
                                                        {"outlets",  "onOrder",  "offOrder", "onDelays", "offDelays", "outlets",  "onOrder",  "offOrder", "onDelays", "offDelays", "outlets",  "onOrder",  "offOrder", "onDelays", "offDelays",  "outlets",   "onOrder",  "offOrder", "onDelays", "offDelays"  },
                                                        {"0",        "0",        "0",        "100",      "120",       "1",        "0",        "0",        "105",      "130",       "2",        "0",        "0",        "107",      "132",        "3",          "0",        "0",        "108",      "133"});

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 4);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;

         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 0 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 100);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 120);

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 1 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 105);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 130);

         outlets = pdt.channelOutlets("channel3");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 2 );

         onOrder = pdt.channelOnOrder("channel3");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel3");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel3");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 107);
         offDelays = pdt.channelOffDelays("channel3");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 132);

         outlets = pdt.channelOutlets("channel4");
         REQUIRE( outlets.size() == 1);
         REQUIRE( outlets[0] == 3 );

         onOrder = pdt.channelOnOrder("channel4");
         REQUIRE( onOrder.size() == 1);
         REQUIRE( onOrder[0] == 0);
         offOrder = pdt.channelOffOrder("channel4");
         REQUIRE( offOrder.size() == 1);
         REQUIRE( offOrder[0] == 0);
         onDelays = pdt.channelOnDelays("channel4");
         REQUIRE( onDelays.size() == 1);
         REQUIRE( onDelays[0] == 108);
         offDelays = pdt.channelOffDelays("channel4");
         REQUIRE( offDelays.size() == 1);
         REQUIRE( offDelays[0] == 133);
      }
   }

   GIVEN("a config file with 2 channels for 4 outlets")
   {
      WHEN("using outlet keyword, only outlet specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",     "channel2" },
                                                        {"outlet",       "outlet"   },
                                                        {"0,1",             "2,3"   } );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 2);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 0 );
         REQUIRE( outlets[1] == 1 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 0);

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 2 );
         REQUIRE( outlets[1] == 3 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 0);


      }

      WHEN("using outlet keyword, all specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1","channel1", "channel1", "channel1", "channel1",  "channel2", "channel2", "channel2", "channel2", "channel2"   },
                                                        {"outlet",  "onOrder",  "offOrder", "onDelays", "offDelays", "outlet",   "onOrder",  "offOrder", "onDelays", "offDelays"  },
                                                        {"0,1",     "0,1",      "1,0",      "0,105",    "0,107",     "2,3",      "1,0",      "0,1",      "0,106",    "0,108"      } );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 2);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 0 );
         REQUIRE( outlets[1] == 1 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 2);
         REQUIRE( onOrder[0] == 0 );
         REQUIRE( onOrder[1] == 1 );
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 2);
         REQUIRE( offOrder[0] == 1 );
         REQUIRE( offOrder[1] == 0 );
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 2);
         REQUIRE( onDelays[0] == 0 );
         REQUIRE( onDelays[1] == 105 );
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 2);
         REQUIRE( offDelays[0] == 0 );
         REQUIRE( offDelays[1] == 107 );

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 2 );
         REQUIRE( outlets[1] == 3 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 2);
         REQUIRE( onOrder[0] == 1 );
         REQUIRE( onOrder[1] == 0 );
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 2);
         REQUIRE( offOrder[0] == 0 );
         REQUIRE( offOrder[1] == 1 );
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 2);
         REQUIRE( onDelays[0] == 0 );
         REQUIRE( onDelays[1] == 106 );
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 2);
         REQUIRE( offDelays[0] == 0 );
         REQUIRE( offDelays[1] == 108 );
      }

      WHEN("using outlets keyword, only outlet specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",     "channel2" },
                                                        {"outlets",       "outlets"   },
                                                        {"0,1",             "2,3"   } );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 2);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 0 );
         REQUIRE( outlets[1] == 1 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 0);


         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 2 );
         REQUIRE( outlets[1] == 3 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 0);
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 0);
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 0);
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 0);
      }

      WHEN("using outlets keyword, all specified")
      {
         mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1","channel1", "channel1", "channel1", "channel1",  "channel2", "channel2", "channel2", "channel2", "channel2"   },
                                                        {"outlets",  "onOrder",  "offOrder", "onDelays", "offDelays", "outlets",   "onOrder",  "offOrder", "onDelays", "offDelays"  },
                                                        {"0,2",     "0,1",      "1,0",      "0,105",    "0,107",     "1,3",      "1,0",      "0,1",      "0,106",    "0,108"      } );

         mx::app::appConfigurator config;
         config.readConfig("/tmp/outletController_test.conf");

         outletControllerTest pdt;
         int rv;
         rv = pdt.setupConfig(config);
         REQUIRE( rv == 0);

         rv = pdt.loadConfig(config);
         REQUIRE( rv == 0);
         REQUIRE( pdt.numChannels() == 2);

         std::vector<size_t> outlets, onOrder, offOrder;
         std::vector<unsigned> onDelays, offDelays;
         outlets = pdt.channelOutlets("channel1");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 0 );
         REQUIRE( outlets[1] == 2 );

         onOrder = pdt.channelOnOrder("channel1");
         REQUIRE( onOrder.size() == 2);
         REQUIRE( onOrder[0] == 0 );
         REQUIRE( onOrder[1] == 1 );
         offOrder = pdt.channelOffOrder("channel1");
         REQUIRE( offOrder.size() == 2);
         REQUIRE( offOrder[0] == 1 );
         REQUIRE( offOrder[1] == 0 );
         onDelays = pdt.channelOnDelays("channel1");
         REQUIRE( onDelays.size() == 2);
         REQUIRE( onDelays[0] == 0 );
         REQUIRE( onDelays[1] == 105 );
         offDelays = pdt.channelOffDelays("channel1");
         REQUIRE( offDelays.size() == 2);
         REQUIRE( offDelays[0] == 0 );
         REQUIRE( offDelays[1] == 107 );

         outlets = pdt.channelOutlets("channel2");
         REQUIRE( outlets.size() == 2);
         REQUIRE( outlets[0] == 1 );
         REQUIRE( outlets[1] == 3 );

         onOrder = pdt.channelOnOrder("channel2");
         REQUIRE( onOrder.size() == 2);
         REQUIRE( onOrder[0] == 1 );
         REQUIRE( onOrder[1] == 0 );
         offOrder = pdt.channelOffOrder("channel2");
         REQUIRE( offOrder.size() == 2);
         REQUIRE( offOrder[0] == 0 );
         REQUIRE( offOrder[1] == 1 );
         onDelays = pdt.channelOnDelays("channel2");
         REQUIRE( onDelays.size() == 2);
         REQUIRE( onDelays[0] == 0 );
         REQUIRE( onDelays[1] == 106 );
         offDelays = pdt.channelOffDelays("channel2");
         REQUIRE( offDelays.size() == 2);
         REQUIRE( offDelays[0] == 0 );
         REQUIRE( offDelays[1] == 108 );
      }
   }
}

/// outletController Operation
/**
 * \ingroup outletController_tests
 */
SCENARIO( "outletController Operation", "[outletController]" )
{
   GIVEN("a config file with 4 channels for 4 outlets, only outlet specified")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", 
                                {"channel1", "channel2",     "channel3",      "channel4"},
                                {"outlet",   "outlet",       "outlet",          "outlet"},
                                {"0",         "1",             "2",                "3"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup outlet states")
      {
         //Verify outlet startup state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );
      }

      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel3
         pdt.turnChannelOn("channel3");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 2 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn off channel3
         pdt.turnChannelOff("channel3");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel4
         pdt.turnChannelOn("channel4");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 2 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 2 );

         //Turn off channel4
         pdt.turnChannelOff("channel4");

         //Verify outlet startup state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );
      }

      WHEN("operating multiple channels")
      {
         //Turn on channel1&2
         pdt.turnChannelOn("channel1");
         pdt.turnChannelOn("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 2 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn off channel1&2
         pdt.turnChannelOff("channel1");
         pdt.turnChannelOff("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel3&4
         pdt.turnChannelOn("channel3");
         pdt.turnChannelOn("channel4");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 2 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 2 );
         REQUIRE( pdt.channelState("channel4") == 2 );

         //Turn off channel3&4
         pdt.turnChannelOff("channel3");
         pdt.turnChannelOff("channel4");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel1&3
         pdt.turnChannelOn("channel1");
         pdt.turnChannelOn("channel3");

         //Verify outlet state
         REQUIRE( pdt.m_outletStates[0] == 2);
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 2 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn off channel1&3
         pdt.turnChannelOff("channel1");
         pdt.turnChannelOff("channel3");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         //Turn on channel2&4
         pdt.turnChannelOn("channel2");
         pdt.turnChannelOn("channel4");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 2 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 2 );

         //Turn off channel2&4
         pdt.turnChannelOff("channel2");
         pdt.turnChannelOff("channel4");

         //Verify outlet startup state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );
      }

      WHEN("outlets intermediate")
      {
         pdt.m_outletStates[0] = 1;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 1 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 1 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         pdt.m_outletStates[0] = 0;

         pdt.m_outletStates[1] = 1;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 1 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 1 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         pdt.m_outletStates[1] = 0;

         pdt.m_outletStates[2] = 1;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 1 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 1 );
         REQUIRE( pdt.channelState("channel4") == 0 );

         pdt.m_outletStates[2] = 0;

         pdt.m_outletStates[3] = 1;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 1 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
         REQUIRE( pdt.channelState("channel3") == 0 );
         REQUIRE( pdt.channelState("channel4") == 1 );

         pdt.m_outletStates[3] = 0;
      }

   }

   GIVEN("a config file with 2 channels for 4 outlets, only outlet specified")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",     "channel2" },
                                                        {"outlet",       "outlet"   },
                                                        {"0,1",             "2,3"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup outlet states")
      {
         //Verify outlet startup state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 2 );

         //verify outlet order
         REQUIRE( pdt.m_timestamps[3] > pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating two channels")
      {
         //Turn on channels
         pdt.turnChannelOn("channel1");
         pdt.turnChannelOn("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 2 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel1&2
         pdt.turnChannelOff("channel1");
         pdt.turnChannelOff("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

      }
      WHEN("outlets intermediate")
      {
         pdt.m_outletStates[0] = 2;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         REQUIRE( pdt.channelState("channel1") == 1);
         REQUIRE( pdt.channelState("channel2") == 0);

         pdt.turnChannelOn("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 2 );
         REQUIRE( pdt.outletState(3) == 2 );

         REQUIRE( pdt.channelState("channel1") == 1);
         REQUIRE( pdt.channelState("channel2") == 2);

         pdt.m_outletStates[0] = 0;

         REQUIRE( pdt.channelState("channel1") == 0);
         REQUIRE( pdt.channelState("channel2") == 2);

         pdt.turnChannelOff("channel2");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         REQUIRE( pdt.channelState("channel1") == 0);
         REQUIRE( pdt.channelState("channel2") == 0);


         pdt.m_outletStates[2] = 1;

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 1 );
         REQUIRE( pdt.outletState(3) == 0 );

         REQUIRE( pdt.channelState("channel1") == 0);
         REQUIRE( pdt.channelState("channel2") == 1);

         pdt.turnChannelOn("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 2 );
         REQUIRE( pdt.outletState(1) == 2 );
         REQUIRE( pdt.outletState(2) == 1 );
         REQUIRE( pdt.outletState(3) == 0 );

         REQUIRE( pdt.channelState("channel1") == 2);
         REQUIRE( pdt.channelState("channel2") == 1);

         pdt.m_outletStates[2] = 0;

         REQUIRE( pdt.channelState("channel1") == 2);
         REQUIRE( pdt.channelState("channel2") == 0);

         pdt.turnChannelOff("channel1");

         //Verify outlet state
         REQUIRE( pdt.outletState(0) == 0 );
         REQUIRE( pdt.outletState(1) == 0 );
         REQUIRE( pdt.outletState(2) == 0 );
         REQUIRE( pdt.outletState(3) == 0 );

         REQUIRE( pdt.channelState("channel1") == 0);
         REQUIRE( pdt.channelState("channel2") == 0);
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onOrder specified")
   {
      //Here we are just testing order, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",   "channel2", "channel2" },
                                                        {"outlet",  "onOrder",     "outlet", "onOrder"   },
                                                        {"0,1",     "0,1",        "2,3",     "0,1"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup channel states")
      {
         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[3] > pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onOrder reversed")
   {
      //Here we are just testing order, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",   "channel2", "channel2" },
                                                        {"outlet",  "onOrder",     "outlet", "onOrder"   },
                                                        {"0,1",     "1,0",        "2,3",     "1,0"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup channel states")
      {
         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[0] > pdt.m_timestamps[1]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[2] > pdt.m_timestamps[3]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onOrder and offOrder specified, the same")
   {
      //Here we are just testing order, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",  "channel1",  "channel2", "channel2", "channel2" },
                                                        {"outlet",  "onOrder",  "offOrder",   "outlet", "onOrder", "offOrder"   },
                                                        {"0,1",     "0,1",       "0,1",  "2,3",     "0,1" , "0,1"  } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup channel states")
      {
         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[3] > pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         REQUIRE( pdt.m_timestamps[3] >= pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onOrder and offOrder specified, different")
   {
      //Here we are just testing order, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",  "channel1",  "channel2", "channel2", "channel2" },
                                                        {"outlet",  "onOrder",  "offOrder",   "outlet", "onOrder", "offOrder"   },
                                                        {"0,1",     "0,1",       "1,0",  "2,3",     "0,1" , "1,0"  } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup channel states")
      {
         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );


         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[0] > pdt.m_timestamps[1]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");


         //verify outlet order
         REQUIRE( pdt.m_timestamps[3] > pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[2] > pdt.m_timestamps[3]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onOrder and offOrder specified, different, reversed")
   {
      //Here we are just testing order, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",  "channel1",  "channel2", "channel2", "channel2" },
                                                        {"outlet",  "onOrder",  "offOrder",   "outlet", "onOrder", "offOrder"   },
                                                        {"0,1",     "1,0",       "0,1",  "2,3",     "1,0" , "0,1"  } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("test device startup channel states")
      {
         //Verify channel state at startup
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[0] > pdt.m_timestamps[1]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );


         //Turn off channel1
         pdt.turnChannelOff("channel1");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[1] > pdt.m_timestamps[0]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");


         //verify outlet order
         REQUIRE( pdt.m_timestamps[2] > pdt.m_timestamps[3]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");

         //verify outlet order
         REQUIRE( pdt.m_timestamps[3] > pdt.m_timestamps[2]);

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
}

SCENARIO( "outletController Operation with delays", "[outletController]" )
{
   std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
   std::cout << "[outletController] Testing delays ... \n";
   GIVEN("a config file with 2 channels for 4 outlets, onDelays specified")
   {
      //Here we are just testing delays, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",   "channel2", "channel2" },
                                                        {"outlet",  "onDelays",   "outlet", "onDelays"   },
                                                        {"0,1",     "0,350",        "2,3",     "0,150"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet delay
         REQUIRE( pdt.m_timestamps[1] >= Approx(pdt.m_timestamps[0]+0.350));
         std::cout << "Ch1 On Delay was " << (pdt.m_timestamps[1] - pdt.m_timestamps[0])*1000 << " msec, expected 350.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");
         std::cout << "Ch1 Off Delay was " << (pdt.m_timestamps[1] - pdt.m_timestamps[0])*1000 << " msec, expected ~0.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");


         //verify outlet delay
         REQUIRE( pdt.m_timestamps[3] >= Approx(pdt.m_timestamps[2]+0.150));
         std::cout << "Ch2 On Delay was " << (pdt.m_timestamps[3] - pdt.m_timestamps[2])*1000 << " msec, expected 150.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");
         std::cout << "Ch2 Off Delay was " << (pdt.m_timestamps[3] - pdt.m_timestamps[2])*1000 << " msec, expected ~0.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, offDelays specified")
   {
      //Here we are just testing delays, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel1",   "channel2", "channel2" },
                                                        {"outlet",  "offDelays",   "outlet", "offDelays"   },
                                                        {"0,1",     "0,550",        "2,3",     "0,750"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         std::cout << "Ch1 On Delay was " << (pdt.m_timestamps[1] - pdt.m_timestamps[0])*1000 << " msec, expected ~0.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");

         REQUIRE( pdt.m_timestamps[1] >= Approx(pdt.m_timestamps[0]+0.550));
         std::cout << "Ch1 Off Delay was " << (pdt.m_timestamps[1] - pdt.m_timestamps[0])*1000 << " msec, expected 550.\n";


         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");


         //verify outlet delay
         std::cout << "Ch1 On Delay was " << (pdt.m_timestamps[3] - pdt.m_timestamps[2])*1000 << " msec, expected ~0.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");
         REQUIRE( pdt.m_timestamps[3] >= Approx(pdt.m_timestamps[2]+0.750));
         std::cout << "Ch1 On Delay was " << (pdt.m_timestamps[3] - pdt.m_timestamps[2])*1000 << " msec, expected 750.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   GIVEN("a config file with 2 channels for 4 outlets, onDelays and offDelays specified, off order reversed")
   {
      //Here we are just testing delays, so we don't need to verify outlet state anymore

      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1", "channel1", "channel1", "channel1",  "channel2", "channel2", "channel2",  "channel2", "channel2" },
                                                     {"outlet",   "onOrder",  "onDelays", "offOrder", "offDelays", "outlet",   "onOrder",  "onDelays", "offOrder", "offDelays"   },
                                                     {"0,1",      "0,1",      "0,350",    "1,0",      "0,450",     "2,3",      "0,1",      "0,150",      "1,0",      "0,75"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("operating a single channel")
      {
         //Turn on channel1
         pdt.turnChannelOn("channel1");

         //verify outlet delay
         REQUIRE( pdt.m_timestamps[1] >= Approx(pdt.m_timestamps[0]+0.350));
         std::cout << "Ch1 On Delay was " << (pdt.m_timestamps[1] - pdt.m_timestamps[0])*1000 << " msec, expected 350.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 2 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn off channel1
         pdt.turnChannelOff("channel1");
         REQUIRE( pdt.m_timestamps[0] >= Approx(pdt.m_timestamps[1]+0.450));
         std::cout << "Ch1 Off Delay was " << (pdt.m_timestamps[0] - pdt.m_timestamps[1])*1000 << " msec, expected 450.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );

         //Turn on channel2
         pdt.turnChannelOn("channel2");


         //verify outlet delay
         REQUIRE( pdt.m_timestamps[3] >= Approx(pdt.m_timestamps[2]+0.150));
         std::cout << "Ch2 On Delay was " << (pdt.m_timestamps[3] - pdt.m_timestamps[2])*1000 << " msec, expected 150.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 2 );

         //Turn off channel2
         pdt.turnChannelOff("channel2");
         REQUIRE( pdt.m_timestamps[2] >= Approx(pdt.m_timestamps[3]+0.075));
         std::cout << "Ch2 Off Delay was " << (pdt.m_timestamps[2] - pdt.m_timestamps[3])*1000 << " msec, expected 75.\n";

         //Verify channel state
         REQUIRE( pdt.channelState("channel1") == 0 );
         REQUIRE( pdt.channelState("channel2") == 0 );
      }
   }
   std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
}

/// outletController Configuration Error Handling
/**
 * \ingroup outletController_tests
 *
 * Exercises the error-return branches of loadConfig().
 */
SCENARIO( "outletController Configuration Error Handling", "[outletController]" )
{
   GIVEN("a channel section with the outlet keyword present but no value")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1"},
                                                        {"outlet"},
                                                        {""} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }

   GIVEN("a channel section with an outlet number that is out of range")
   {
      //pdt has 4 outlets (0-3), so outlet 10 is invalid
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1"},
                                                        {"outlet"},
                                                        {"10"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }

   GIVEN("a channel section with a mismatched onOrder size")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1"},
                                                        {"outlet",   "onOrder"},
                                                        {"0,1",      "0"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }

   GIVEN("a channel section with a mismatched offOrder size")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1"},
                                                        {"outlet",   "offOrder"},
                                                        {"0,1",      "0"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }

   GIVEN("a channel section with a mismatched onDelays size")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1"},
                                                        {"outlet",   "onDelays"},
                                                        {"0,1",      "100"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }

   GIVEN("a channel section with a mismatched offDelays size")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1", "channel1"},
                                                        {"outlet",   "offDelays"},
                                                        {"0,1",      "100"} );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      int rv = pdt.setupConfig(config);
      REQUIRE( rv == 0);

      WHEN("loadConfig is called")
      {
         rv = pdt.loadConfig(config);
         REQUIRE( rv == -1);
      }
   }
}

/// outletController updateOutletStates
/**
 * \ingroup outletController_tests
 *
 * Exercises the default updateOutletStates() implementation, both the
 * success path (all outlets update cleanly) and the error path (an outlet
 * reports an error, which should short-circuit the loop).
 */
SCENARIO( "outletController updateOutletStates default implementation", "[outletController]" )
{
   GIVEN("a controller with 4 outlets, all in a valid state")
   {
      outletControllerTest pdt;
      pdt.setNumberOfOutlets(4);

      WHEN("updateOutletStates is called and all outlets report valid states")
      {
         int rv = pdt.updateOutletStates();
         REQUIRE( rv == 0);
      }

      WHEN("updateOutletStates is called and an outlet reports an error")
      {
         pdt.m_outletStates[2] = -7;
         int rv = pdt.updateOutletStates();
         REQUIRE( rv == -7);
         pdt.m_outletStates[2] = 0; //reset
      }
   }
}

/// outletController turnChannelOn/turnChannelOff edge cases
/**
 * \ingroup outletController_tests
 *
 * Exercises the branches of turnChannelOn/turnChannelOff that are not hit
 * by ordinary operation: the null-mutex fallback, the already-on/already-off
 * no-ops, the state-delay skip logic, and the outlet-error return paths.
 */
SCENARIO( "outletController turnChannelOn/turnChannelOff edge cases", "[outletController]" )
{
   GIVEN("a config file with 2 channels for 4 outlets")
   {
      mx::app::writeConfigFile( "/tmp/outletController_test.conf", {"channel1",  "channel2" },
                                                        {"outlet",   "outlet"   },
                                                        {"0,1",      "2,3"   } );

      mx::app::appConfigurator config;
      config.readConfig("/tmp/outletController_test.conf");

      outletControllerTest pdt;
      pdt.setupConfig(config);
      pdt.loadConfig(config);

      WHEN("the channel mutex is null")
      {
         //Force the null-mutex fallback path (channelSpec::m_mutex).  The underlying
         //mutex is still owned and deleted via m_channelMutexes, so this is safe.
         pdt.m_channels["channel1"].m_mutex = nullptr;

         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 2 );

         pdt.m_channels["channel1"].m_mutex = nullptr;
         rv = pdt.turnChannelOff("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 0 );
      }

      WHEN("turning on a channel that is already on")
      {
         pdt.turnChannelOn("channel1");
         REQUIRE( pdt.channelState("channel1") == 2 );

         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 2 );
      }

      WHEN("turning off a channel that is already off")
      {
         REQUIRE( pdt.channelState("channel1") == 0 );

         int rv = pdt.turnChannelOff("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 0 );
      }

      WHEN("the state delay has not elapsed when turning a channel on")
      {
         pdt.m_stateDelay = 5.0; //large delay so it won't elapse during the test

         //First transition: default m_stateTime is old, so this proceeds normally.
         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 2 );

         //Force the channel to appear off without going through turnChannelOff,
         //so m_stateTime remains recent (i.e. within the delay window).
         pdt.m_outletStates[0] = 0;
         pdt.m_outletStates[1] = 0;
         REQUIRE( pdt.channelState("channel1") == 0 );

         //Now attempting to turn it on again should be skipped by the delay logic.
         rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.outletState(0) == 0 ); //unchanged -- turnOutletOn was not actually called
         REQUIRE( pdt.outletState(1) == 0 );
      }

      WHEN("the state delay has not elapsed when turning a channel off")
      {
         pdt.m_stateDelay = 5.0; //large delay so it won't elapse during the test

         //Turn the channel on -- this sets m_stateTime to now.
         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 2 );

         //Immediately attempting to turn it off should be skipped by the delay logic.
         rv = pdt.turnChannelOff("channel1");
         REQUIRE( rv == 0 );
         REQUIRE( pdt.channelState("channel1") == 2 ); //unchanged -- still on
      }

      WHEN("the first outlet fails to turn on")
      {
         pdt.m_failOutlet = 0; //channel1's first outlet

         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == -1 );

         pdt.m_failOutlet = -1; //reset
      }

      WHEN("a subsequent outlet fails to turn on")
      {
         pdt.m_failOutlet = 1; //channel1's second outlet

         int rv = pdt.turnChannelOn("channel1");
         REQUIRE( rv == -1 );
         REQUIRE( pdt.outletState(0) == 2 ); //first outlet succeeded before the failure

         pdt.m_failOutlet = -1; //reset
      }

      WHEN("the first outlet fails to turn off")
      {
         pdt.turnChannelOn("channel1");
         REQUIRE( pdt.channelState("channel1") == 2 );

         pdt.m_failOutlet = 0; //channel1's first outlet

         int rv = pdt.turnChannelOff("channel1");
         REQUIRE( rv == -1 );

         pdt.m_failOutlet = -1; //reset
      }

      WHEN("a subsequent outlet fails to turn off")
      {
         pdt.turnChannelOn("channel1");
         REQUIRE( pdt.channelState("channel1") == 2 );

         pdt.m_failOutlet = 1; //channel1's second outlet

         int rv = pdt.turnChannelOff("channel1");
         REQUIRE( rv == -1 );
         REQUIRE( pdt.outletState(0) == 0 ); //first outlet succeeded before the failure

         pdt.m_failOutlet = -1; //reset
      }
   }
}

/// outletController stateIntToString
/**
 * \ingroup outletController_tests
 *
 * Exercises all four branches (Off/Int/On/Unk) of the free function
 * MagAOX::app::dev::stateIntToString, which is compiled into libMagAOX.a
 * from outletController.cpp (not header-only).
 */
SCENARIO( "outletController stateIntToString", "[outletController]" )
{
   GIVEN("the four possible outlet state values")
   {
      WHEN("the state is OUTLET_STATE_OFF")
      {
         REQUIRE( dev::stateIntToString(OUTLET_STATE_OFF) == "Off" );
      }

      WHEN("the state is OUTLET_STATE_INTERMEDIATE")
      {
         REQUIRE( dev::stateIntToString(OUTLET_STATE_INTERMEDIATE) == "Int" );
      }

      WHEN("the state is OUTLET_STATE_ON")
      {
         REQUIRE( dev::stateIntToString(OUTLET_STATE_ON) == "On" );
      }

      WHEN("the state is an unknown/unexpected value")
      {
         REQUIRE( dev::stateIntToString(OUTLET_STATE_UNKNOWN) == "Unk" );
         REQUIRE( dev::stateIntToString(42) == "Unk" );
      }
   }
}

} //namespace outletController_tests
