/** \file dm_test.hpp
  * \brief Test harnesses for the MagAOX::app::dev::dm device mixin.
  *
  * Declares dmTest, a MagAOXApp<false> that mixes in dm<dmTest, float> and a real
  * shmimMonitor base. The hardware hooks initDM(), zeroDM(), releaseDM(), and commandDM()
  * are stubs whose return values a test can set. The harness exposes protected dm<> state,
  * the INDI properties and callbacks, and the saturation thread bookkeeping so tests can
  * drive each branch directly without a full appStartup(). It also exposes a few
  * MagAOXApp<false> functions that MagAOXApp_test.cpp cannot reach, because that file only
  * instantiates MagAOXApp<true>.
  *
  * Also declares dmTestBadType, a minimal harness with an unsupported DM data type.
  *
  * \ingroup dm_tests
  */

#include "../../MagAOXApp.hpp"
#include "../dm.hpp"
#include "../shmimMonitor.hpp"
#include "testHarnessCommon.hpp"

// LCOV_EXCL_START

#ifndef XWCTEST_NAMESPACE
    #define MAPPNS MagAOX::app::dev
#else
    #define MAPPNS MagAOX::app::dev::XWCTEST_NAMESPACE

#endif

namespace dm_tests
{

#ifdef XWCTEST_NAMESPACE
namespace XWCTEST_NAMESPACE
{
#endif

/// Test harness for dev::dm
/**
 * \ingroup dm_tests
 */
struct dmTest : public MagAOX::app::MagAOXApp<false>,
                public MAPPNS::dm<dmTest, float>,
                public MAPPNS::shmimMonitor<dmTest>
{

    friend class MAPPNS::dm<dmTest, float>;

    typedef MAPPNS::dm<dmTest, float> dmT;

    dmTest( const std::string &git_sha1, const bool git_modified )
        : MagAOX::app::MagAOXApp<false>( git_sha1, git_modified )
    {
        m_configName  = "dmtest";
        m_calibDir    = "/tmp/dmtest_calibs";
        m_calibRelDir = "dmtest";
    }

    ~dmTest() noexcept
    {
    }

    /// Set the config name and attach a real indiDriver that has no FIFOs.
    /// Afterwards m_indiDriver is not null, so the code paths that send INDI updates run.
    /// Helpers such as indi::updateIfChanged() catch their own send failures, so no live
    /// INDI server is needed. shmimMonitor_test.cpp and frameGrabber_test.cpp use the
    /// same pattern.
    void setConfigNameWithDriver( const std::string &cn )
    {
        m_configName = cn;
        m_indiDriver = MagAOX::app::dev::testHarness::makeFifolessIndiDriver<MagAOX::app::MagAOXApp<false>>(
            this, m_configName );
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return dmT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return dmT::loadConfig( config );
    }

    int appStartup()
    {
        return dmT::appStartup();
    }

    int appLogic()
    {
        return dmT::appLogic();
    }

    int appShutdown()
    {
        return dmT::appShutdown();
    }

    /// The next three functions forward the MagAOXApp hooks to the dm mixin so tests
    /// can call them directly.
    int onPowerOff()
    {
        return dmT::onPowerOff();
    }

    int whilePowerOff()
    {
        return dmT::whilePowerOff();
    }

    int updateINDI()
    {
        return dmT::updateINDI();
    }

    /// Return values of the stub hardware hooks initDM(), zeroDM(), and releaseDM().
    /// A test sets one of them to -1 to make that hook fail.
    int m_initDMRV{ 0 };
    int initDM()
    {
        return m_initDMRV;
    }

    int m_zeroDMRV{ 0 };
    int zeroDM()
    {
        return m_zeroDMRV;
    }

    int m_releaseDMRV{ 0 };
    int releaseDM()
    {
        return m_releaseDMRV;
    }

    void setSize( int w, int h, int d )
    {
        m_width    = w;
        m_height   = h;
        m_dataType = d;
    }

    /// Value written into m_instSatMap by commandDM(), settable by tests.
    uint8_t m_testSatValue{ 0 };

    /// If true, commandDM() returns -1 to exercise the processImage() error path.
    bool m_commandDMFail{ false };

    /// Stub for the hardware hook that dm<>::processImage() calls to command the DM.
    /// It fills the instantaneous saturation map with m_testSatValue, or returns -1
    /// when m_commandDMFail is set.
    int commandDM( void *curr_src )
    {
        static_cast<void>( curr_src );
        if( m_commandDMFail )
        {
            return -1;
        }
        if( m_instSatMap.rows() > 0 && m_instSatMap.cols() > 0 )
        {
            m_instSatMap.setConstant( m_testSatValue );
        }
        return 0;
    }

    // Test-only accessors for dm<> state that is otherwise protected.

    size_t numFlatCommands()
    {
        return m_flatCommands.size();
    }

    size_t numTestCommands()
    {
        return m_testCommands.size();
    }

    bool flatLoaded()
    {
        return m_flatLoaded;
    }

    bool testLoaded()
    {
        return m_testLoaded;
    }

    bool flatIsSet()
    {
        return m_flatSet;
    }

    bool testIsSet()
    {
        return m_testSet;
    }

    const std::string &flatCurrent()
    {
        return m_flatCurrent;
    }

    const std::string &testCurrent()
    {
        return m_testCurrent;
    }

    pcf::IndiProperty indiP_init()
    {
        return m_indiP_init;
    }

    pcf::IndiProperty indiP_zero()
    {
        return m_indiP_zero;
    }

    pcf::IndiProperty indiP_release()
    {
        return m_indiP_release;
    }

    pcf::IndiProperty indiP_zeroAll()
    {
        return m_indiP_zeroAll;
    }

    pcf::IndiProperty indiP_setFlat()
    {
        return m_indiP_setFlat;
    }

    pcf::IndiProperty indiP_setTest()
    {
        return m_indiP_setTest;
    }

    pcf::IndiProperty indiP_flats()
    {
        return m_indiP_flats;
    }

    pcf::IndiProperty indiP_tests()
    {
        return m_indiP_tests;
    }

    /// Build the request and toggle INDI properties that appStartup() normally creates.
    /// The rest of appStartup() is skipped, so no thread or semaphore is created.
    /// This lets tests call the newCallBack_* functions directly.
    void prepIndiForCallbackTests()
    {
        createStandardIndiToggleSw( m_indiP_setFlat, "flat_set" );
        createStandardIndiToggleSw( m_indiP_setTest, "test_set" );
        createStandardIndiRequestSw( m_indiP_init, "initDM" );
        createStandardIndiRequestSw( m_indiP_zero, "zeroDM" );
        createStandardIndiRequestSw( m_indiP_release, "releaseDM" );
        createStandardIndiRequestSw( m_indiP_zeroAll, "zeroAll" );
    }

    /// Fill both saturation maps with nonzero values so a later clearSat() has
    /// something to clear.
    void setAccumSatNonzero()
    {
        m_accumSatMap.setConstant( 3 );
        m_instSatMap.setConstant( 1 );
    }

    /// Set the shutdown flag the way a signal handler would.
    void requestShutdown()
    {
        m_shutdown = 1;
    }

    /// Set the power management flag directly.
    /// This reaches the critical-shutdown branch of MagAOXApp<false>::setupBasicConfig()
    /// for power management enabled without INDI. MagAOXApp_test.cpp only instantiates
    /// MagAOXApp<true>, so that branch cannot be reached from there.
    void setPowerMgtEnabled( bool pme )
    {
        m_powerMgtEnabled = pme;
    }

    /// Call MagAOXApp<false>::startINDI(), which trivially succeeds when _useINDI is
    /// false. This branch is also unreachable from MagAOXApp_test.cpp.
    int startINDI()
    {
        return MagAOX::app::MagAOXApp<false>::startINDI();
    }

    /// Call MagAOXApp<false>::createINDIFIFOS(), which trivially succeeds when INDI is
    /// compiled out. This branch is also unreachable from MagAOXApp_test.cpp.
    int callCreateINDIFIFOS()
    {
        return MagAOX::app::MagAOXApp<false>::createINDIFIFOS();
    }

    /// The next three functions call the INDI handle* callbacks of MagAOXApp<false>.
    /// With m_useINDI false at compile time each callback returns immediately. These
    /// branches are also unreachable from MagAOXApp_test.cpp, which only instantiates
    /// MagAOXApp<true>.
    void callHandleGetProperties( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleGetProperties( ipRecv );
    }

    void callHandleNewProperty( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleNewProperty( ipRecv );
    }

    void callHandleSetProperty( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleSetProperty( ipRecv );
    }

    /// Call the registerIndiPropertyNew() overload that builds the property from a type,
    /// a permission, and a state, without a switch rule. No dev:: mixin uses this
    /// overload and MagAOXApp_test.cpp only instantiates MagAOXApp<true>, so it is
    /// otherwise untested.
    int callRegisterIndiPropertyNew( pcf::IndiProperty &prop, const std::string &propName )
    {
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyNew(
            prop, propName, pcf::IndiProperty::Number, pcf::IndiProperty::ReadWrite, pcf::IndiProperty::Idle, nullptr );
    }

    /// Call MagAOXApp<false>::sendNewStandardIndiToggle(), which trivially succeeds
    /// when _useINDI is false at compile time. This branch is also unreachable from
    /// MagAOXApp_test.cpp.
    int callSendNewStandardIndiToggle( const std::string &device, const std::string &property, bool onoff )
    {
        return MagAOX::app::MagAOXApp<false>::sendNewStandardIndiToggle( device, property, onoff );
    }

    /// Same as callRegisterIndiPropertyNew() but uses the overload that also takes an
    /// explicit switch rule.
    int callRegisterIndiPropertyNewWithRule( pcf::IndiProperty &prop, const std::string &propName )
    {
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyNew( prop,
                                                                       propName,
                                                                       pcf::IndiProperty::Switch,
                                                                       pcf::IndiProperty::ReadWrite,
                                                                       pcf::IndiProperty::Idle,
                                                                       pcf::IndiProperty::OneOfMany,
                                                                       nullptr );
    }

    /// Initialize the saturation semaphore with sem_init() without running the rest of
    /// appStartup(). processImage() posts this semaphore, so it must be valid when tests
    /// call processImage() directly.
    void initSatSemaphoreForTest()
    {
        sem_init( &m_satSemaphore, 0, 0 );
    }

    /// Replace the saturation thread object with a caller-supplied std::thread.
    /// This lets a test reach the appLogic() branch for a saturation thread that has
    /// exited without ever starting the real thread. Tearing down the real thread is
    /// hazardous, as explained in dm_test.cpp. setSmThread() and abandonSmThread() in
    /// shmimMonitor_test.cpp follow the same pattern.
    void setSatThread( std::thread &&t )
    {
        m_satThread = std::move( t );
    }

    /// Swap the saturation thread object out and overwrite the copy with an empty
    /// thread. The handle is leaked on purpose. appLogic() has already reaped the thread
    /// with pthread_tryjoin_np(), and destroying a joinable std::thread would call
    /// std::terminate().
    void abandonSatThread()
    {
        std::thread tmp;
        tmp.swap( m_satThread );
        new( &tmp ) std::thread();
    }
};

/// A minimal harness that uses int as the DM data type.
/**
 * int has no ImageStreamTypeCode<> specialization, so m_dmDataType is 0 and
 * appStartup() must reject it. This reaches the unsupported data type error path.
 * The hooks initDM(), zeroDM(), releaseDM(), and commandDM() are stubs that succeed.
 *
 * \ingroup dm_tests
 */
struct dmTestBadType : public MagAOX::app::MagAOXApp<false>,
                       public MAPPNS::dm<dmTestBadType, int>,
                       public MAPPNS::shmimMonitor<dmTestBadType>
{

    friend class MAPPNS::dm<dmTestBadType, int>;

    typedef MAPPNS::dm<dmTestBadType, int> dmT;

    dmTestBadType( const std::string &git_sha1, const bool git_modified )
        : MagAOX::app::MagAOXApp<false>( git_sha1, git_modified )
    {
        m_configName  = "dmtestbadtype";
        m_calibDir    = "/tmp/dmtestbadtype_calibs";
        m_calibRelDir = "dmtestbadtype";
    }

    ~dmTestBadType() noexcept
    {
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return dmT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return dmT::loadConfig( config );
    }

    int appStartup()
    {
        return dmT::appStartup();
    }

    int appLogic()
    {
        return 0;
    }

    int appShutdown()
    {
        return 0;
    }

    int initDM()
    {
        return 0;
    }

    int zeroDM()
    {
        return 0;
    }

    int releaseDM()
    {
        return 0;
    }

    int commandDM( void *curr_src )
    {
        static_cast<void>( curr_src );
        return 0;
    }
};

#ifdef XWCTEST_NAMESPACE
} // namespace XWCTEST_NAMESPACE
#endif

} // namespace dm_tests

// LCOV_EXCL_STOP
